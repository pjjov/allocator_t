/** `allocator_t` - Interface for custom allocators in C.

    This files provides an allocator that pools allocations of wanted size and
    alignment into large blocks chained in a linked list. The pool is a great
    alternative to dynamic arrays when pointer stability is required. This makes
    it fit for ECS and actor systems.

    SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0

    Copyright 2025-2026 Предраг Јовановић

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
**/

#ifndef ALLOCATOR_POOL_H
#define ALLOCATOR_POOL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ALLOCATOR_H
    #include "allocator.h"
#endif

#include <assert.h>
#include <stdint.h>
#include <string.h>

enum pool_allocator_flags {
    /** Passes allocations that don't fit a slot -- and, once maxBlocks is
        reached, allocations that don't fit any existing block either --
        straight through to the base allocator instead of failing. */
    POOL_ALLOC_ALLOW_UNFIT = 1,
    /** Frees a block's backing buffer as soon as it becomes empty,
        instead of keeping it around for reuse. */
    POOL_ALLOC_FREE_EMPTY = 2,
};

/** One chunk of the pool: `capacity` slots, a bitset tracking which are
    used, and the slot data itself. Blocks are chained so the pool can
    grow past its initial capacity instead of overflowing to the base
    allocator (or failing) once the first block fills up. */
struct pool_block {
    struct pool_block *next;
    struct pool_block *prev;

    size_t capacity;
    size_t count;

    /* word index likely to contain a free bit within *this* block;
       avoids rescanning from the start of the bitset every allocation */
    size_t hint;

    /* bitset (1 bit per slot, 1 == used) immediately followed by the
       slot data itself, both in one allocation from `base`. */
    uint64_t *meta;
};

typedef struct pool_allocator_t {
    allocator_t interface;
    allocator_t *base;

    size_t blockCapacity; /* slots per block */
    size_t maxBlocks;     /* 0 == unlimited growth; otherwise caps blockCount */
    size_t blockCount;    /* number of blocks currently linked into `blocks` */
    size_t count;         /* slots in use, summed across all blocks */

    size_t itemSize; /* per-slot stride, already rounded up to itemAlign */
    size_t itemAlign;

    unsigned flags; /* pool_allocator_flags */

    /* Block most likely to have a free slot -- checked first on alloc,
       so steady-state alloc/free doesn't have to walk the block list.
       NULL means "no known candidate, scan from the head". */
    struct pool_block *freeHint;

    /* Head of the block list. New blocks are prepended, since a
       freshly-created block is guaranteed to have free slots. */
    struct pool_block *blocks;
} pool_allocator_t;

#define POOL__BITS_PER_WORD (sizeof(uint64_t) * 8)

static inline size_t pool__align_up(size_t n, size_t align) {
    if (align < 1)
        align = 1;
    return (n + (align - 1)) & ~(align - 1);
}

/** pool__align_up uses a bitmask trick that only works for power-of-two
    alignments; round whatever the caller passed up to the next one so a
    non-power-of-two itemAlign (e.g. 3) can't silently corrupt offsets. */
static inline size_t pool__round_pow2(size_t n) {
    size_t p = 1;
    if (n <= 1)
        return 1;
    while (p < n)
        p <<= 1;
    return p;
}

static inline size_t pool__words(size_t capacity) {
    return (capacity + POOL__BITS_PER_WORD - 1) / POOL__BITS_PER_WORD;
}

static inline size_t pool__ctz64(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
    return (size_t)__builtin_ctzll(x);
#else
    size_t n = 0;
    while (!(x & 1)) {
        x >>= 1;
        n++;
    }
    return n;
#endif
}

static inline int pool__test(struct pool_block *block, size_t index) {
    return (int)((block->meta[index / POOL__BITS_PER_WORD]
                  >> (index % POOL__BITS_PER_WORD))
                 & 1);
}

static inline void pool__set(struct pool_block *block, size_t index) {
    block->meta[index / POOL__BITS_PER_WORD]
        |= ((uint64_t)1 << (index % POOL__BITS_PER_WORD));
}

static inline void pool__clear(struct pool_block *block, size_t index) {
    block->meta[index / POOL__BITS_PER_WORD] &= ~(
        (uint64_t)1 << (index % POOL__BITS_PER_WORD)
    );
}

/** Finds the first unused slot in `block`, or (size_t)-1 if it's full.
    Starts at `block->hint` instead of word 0: after a free(), hint points
    at a word that's guaranteed to have a free bit, so the common
    alloc/free churn pattern costs one word test instead of a rescan. */
static size_t pool__find_free_in(struct pool_block *block) {
    size_t words = pool__words(block->capacity);
    size_t start = block->hint < words ? block->hint : 0;

    for (size_t k = 0; k < words; k++) {
        size_t w = start + k;
        if (w >= words)
            w -= words; /* wrap around to the front */

        uint64_t word = block->meta[w];
        if (word == UINT64_MAX)
            continue;

        size_t bit = pool__ctz64(~word);
        size_t index = w * POOL__BITS_PER_WORD + bit;
        if (index >= block->capacity)
            continue; /* trailing padding bits */

        block->hint = w; /* next search starts here too */
        return index;
    }
    return (size_t)-1;
}

/** Byte offset of slot data from the start of a block's meta buffer. */
static inline size_t pool__data_offset(
    pool_allocator_t *pool, size_t capacity
) {
    size_t bitsetBytes = pool__words(capacity) * sizeof(uint64_t);
    return pool__align_up(bitsetBytes, pool->itemAlign);
}

static inline void *pool__slot(
    pool_allocator_t *pool, struct pool_block *block, size_t index
) {
    return (char *)block->meta + pool__data_offset(pool, block->capacity)
        + index * pool->itemSize;
}

/** Total size of the single allocation backing a block of this capacity
    (bitset + slot data together). */
static inline size_t pool__block_bytes(
    pool_allocator_t *pool, size_t capacity
) {
    return pool__data_offset(pool, capacity) + pool->itemSize * capacity;
}

/** Whether a request of this size/align can live in one slot. */
static inline int pool__fits(
    pool_allocator_t *pool, size_t size, size_t align
) {
    if (size > pool->itemSize)
        return 0;
    if (align > 1 && (pool->itemAlign % align) != 0)
        return 0;
    return 1;
}

/** Finds the block `ptr` was carved out of, and its slot index within
    that block. Returns NULL if `ptr` doesn't belong to any block (i.e.
    it must have overflowed to the base allocator). */
static struct pool_block *pool__find_owner(
    pool_allocator_t *pool, void *ptr, size_t *outIndex
) {
    if (!ptr)
        return NULL;

    for (struct pool_block *block = pool->blocks; block; block = block->next) {
        char *base = (char *)pool__slot(pool, block, 0);
        char *p = (char *)ptr;
        if (p < base)
            continue;

        size_t off = (size_t)(p - base);
        if (off % pool->itemSize != 0)
            continue;

        size_t index = off / pool->itemSize;
        if (index >= block->capacity)
            continue;

        *outIndex = index;
        return block;
    }
    return NULL;
}

/** Allocates and links in a fresh block of pool->blockCapacity slots.
    Returns NULL (and leaves the pool untouched) on allocation failure. */
static struct pool_block *pool__new_block(pool_allocator_t *pool) {
    size_t capacity = pool->blockCapacity;
    size_t bitsetBytes = pool__words(capacity) * sizeof(uint64_t);
    size_t total = pool__block_bytes(pool, capacity);

    /* The bitset is accessed as uint64_t, so the buffer needs at least
       that alignment -- itemAlign alone isn't enough when it's smaller
       (e.g. itemAlign == 1 for a pool of bytes), which would otherwise
       hand back a misaligned uint64_t* and cause misaligned/undefined
       accesses in pool__test/set/clear. */
    size_t metaAlign = pool->itemAlign > sizeof(uint64_t) ? pool->itemAlign
                                                          : sizeof(uint64_t);

    struct pool_block *block = (struct pool_block *)allocator_call(
        pool->base, NULL, 0, sizeof(*block), 0
    );
    if (!block)
        return NULL;

    uint64_t *meta = (uint64_t *)allocator_call(
        pool->base, NULL, 0, total, metaAlign
    );
    if (!meta) {
        allocator_call(pool->base, block, sizeof(*block), 0, 0);
        return NULL;
    }

    memset(meta, 0, bitsetBytes);

    block->next = NULL;
    block->prev = NULL;
    block->capacity = capacity;
    block->count = 0;
    block->hint = 0;
    block->meta = meta;
    return block;
}

/** Releases a block's backing buffer and its header back to the base
    allocator. Caller must have already unlinked it from the list. */
static void pool__free_block(pool_allocator_t *pool, struct pool_block *block) {
    if (block->meta) {
        allocator_call(
            pool->base,
            block->meta,
            pool__block_bytes(pool, block->capacity),
            0,
            0
        );
    }
    allocator_call(pool->base, block, sizeof(*block), 0, 0);
}

static void pool__unlink_block(
    pool_allocator_t *pool, struct pool_block *block
) {
    if (block->prev)
        block->prev->next = block->next;
    else
        pool->blocks = block->next;

    if (block->next)
        block->next->prev = block->prev;

    pool->blockCount--;
}

/** Releases slot `index` in `block` back to the pool: clears its bit,
    updates counts/hints, and -- if the block just went empty and
    POOL_ALLOC_FREE_EMPTY is set -- unlinks and frees the whole block. */
static void pool__release(
    pool_allocator_t *pool, struct pool_block *block, size_t index
) {
    /* If the bit is already clear, the caller is freeing (or reallocating
       away) a pointer that isn't currently allocated -- a double free.
       Clearing it again would still be a no-op on the bitset, but the
       count-- below would silently underflow and corrupt this block's
       (and the pool's) bookkeeping, so catch it here instead. */
    assert(
        pool__test(block, index)
        && "pool_allocator: double free or invalid pointer"
    );

    pool__clear(block, index);
    block->count--;
    pool->count--;
    /* this word now definitely has a free bit -- next alloc in this
       block can jump straight to it instead of scanning */
    block->hint = index / POOL__BITS_PER_WORD;

    if (block->count == 0 && (pool->flags & POOL_ALLOC_FREE_EMPTY)) {
        if (pool->freeHint == block)
            pool->freeHint = NULL;
        pool__unlink_block(pool, block);
        pool__free_block(pool, block);
    } else {
        /* this block now definitely has a free slot -- next alloc can
           try it first instead of walking the block list */
        pool->freeHint = block;
    }
}

static void pool__free_all(pool_allocator_t *pool) {
    struct pool_block *block = pool->blocks;
    while (block) {
        struct pool_block *next = block->next;
        pool__free_block(pool, block);
        block = next;
    }
    pool->blocks = NULL;
    pool->freeHint = NULL;
    pool->blockCount = 0;
    pool->count = 0;
}

static void *pool_allocator_fn(
    allocator_t *self, void *ptr, size_t old, size_t size, size_t zalign
) {
    if (!self)
        return NULL;

    pool_allocator_t *pool = (pool_allocator_t *)self;
    int clearBuffer = zalign & 1;
    size_t align = zalign & ~(size_t)1;

    if (self == ptr) {
        pool__free_all(pool);
        return NULL;
    }

    if (ptr) {
        size_t index;
        struct pool_block *block = pool__find_owner(pool, ptr, &index);

        if (block) {
            if (size == 0) {
                /** free */
                pool__release(pool, block, index);
                return NULL;
            }

            if (pool__fits(pool, size, align)) {
                /** slot is reused in place, it's already the right size */
                if (clearBuffer && size > old) {
                    memset((char *)ptr + old, 0, size - old);
                }
                return ptr;
            }

            /** grew past what a slot can hold: migrate it out of the pool */
            if (!(pool->flags & POOL_ALLOC_ALLOW_UNFIT)) {
                return NULL;
            }

            void *bigger = allocator_call(pool->base, NULL, 0, size, zalign);
            if (!bigger)
                return NULL;

            memcpy(bigger, ptr, old < size ? old : size);
            pool__release(pool, block, index);
            return bigger;
        }

        /** Not one of our slots: it must be a block that previously
            overflowed to the base allocator because it didn't fit. */
        if (!(pool->flags & POOL_ALLOC_ALLOW_UNFIT)) {
            return NULL;
        }
        return allocator_call(pool->base, ptr, old, size, zalign);
    }

    /** allocate */
    if (size == 0) {
        return NULL;
    }

    if (pool__fits(pool, size, align)) {
        struct pool_block *b = pool->freeHint;
        size_t index = b ? pool__find_free_in(b) : (size_t)-1;

        if (index == (size_t)-1) {
            /* hint missed (or there wasn't one): fall back to scanning
               every block. */
            for (b = pool->blocks; b; b = b->next) {
                if (b == pool->freeHint)
                    continue; /* already tried above */
                index = pool__find_free_in(b);
                if (index != (size_t)-1)
                    break;
            }
        }

        if (index != (size_t)-1) {
            pool__set(b, index);
            b->count++;
            pool->count++;
            pool->freeHint = (b->count < b->capacity) ? b : NULL;

            void *result = pool__slot(pool, b, index);
            if (clearBuffer) {
                memset(result, 0, pool->itemSize);
            }
            return result;
        }

        /** every existing block is full: grow, unless capped */
        if (pool->maxBlocks == 0 || pool->blockCount < pool->maxBlocks) {
            struct pool_block *nb = pool__new_block(pool);
            if (nb) {
                nb->next = pool->blocks;
                if (pool->blocks)
                    pool->blocks->prev = nb;
                pool->blocks = nb;
                pool->blockCount++;

                index = pool__find_free_in(
                    nb
                ); /* fresh block: always succeeds */
                pool__set(nb, index);
                nb->count++;
                pool->count++;
                pool->freeHint = (nb->count < nb->capacity) ? nb : NULL;

                void *result = pool__slot(pool, nb, index);
                if (clearBuffer) {
                    memset(result, 0, pool->itemSize);
                }
                return result;
            }
            /* allocating the new block itself failed -- fall through to
               the unfit/overflow handling below just like a full pool. */
        }
    }

    /** too big for a slot, or the pool is full and can't (or won't) grow */
    if (pool->flags & POOL_ALLOC_ALLOW_UNFIT) {
        return allocator_call(pool->base, NULL, 0, size, zalign);
    }
    return NULL;
}

/** Initializes a pool allocator. Slots are itemSize bytes (rounded up to
    itemAlign) each; blocks of `capacity` slots are allocated from `base`
    as needed, starting with one eagerly here. Once a block fills up, a
    new one is transparently chained on rather than falling back to
    `base` for individual allocations, so the pool can absorb far more
    allocations than a single block's capacity without giving up pooling.

    Growth is unlimited by default. To cap it (after which the pool
    behaves like the old fixed-capacity version -- POOL_ALLOC_ALLOW_UNFIT
    decides whether overflow allocations go to `base` or simply fail),
    set `pool->maxBlocks` to a non-zero limit any time after this call
    returns successfully.

    Returns non-zero on success. */
static int pool_allocator_init(
    pool_allocator_t *pool,
    allocator_t *base,
    size_t capacity,
    size_t itemSize,
    size_t itemAlign,
    unsigned flags
) {
    if (!pool || !capacity || !itemSize)
        return 0;

    /* pool__align_up (used below and for every slot offset) relies on
       align being a power of two; enforce that instead of silently
       mis-aligning slots for e.g. itemAlign == 3. */
    itemAlign = pool__round_pow2(itemAlign);

    size_t stride = pool__align_up(itemSize, itemAlign);

    /* Guard against capacity * stride overflowing size_t, which would
       otherwise make pool__new_block() allocate a buffer far smaller
       than the block believes it has. */
    if (stride != 0 && capacity > (SIZE_MAX / stride))
        return 0;

    pool_allocator_t tmp = {
        .interface = { pool_allocator_fn },
        .base = base,
        .blockCapacity = capacity,
        .maxBlocks = 0, /* unlimited growth by default */
        .blockCount = 0,
        .count = 0,
        .itemSize = stride,
        .itemAlign = itemAlign,
        .flags = flags,
        .freeHint = NULL,
        .blocks = NULL,
    };

    memcpy(pool, &tmp, sizeof(tmp));

    struct pool_block *block = pool__new_block(pool);
    if (!block)
        return 0;

    pool->blocks = block;
    pool->freeHint = block;
    pool->blockCount = 1;
    return 1;
}

#ifdef __cplusplus
}
#endif

#endif