/** `allocator_t` - Interface for custom allocators in C.

    This file provides a wrapping region allocator, also known as arena.
    Arenas allocate buffers sequentially, which enables fast allocation
    times at the expense of higher memory consumption. Arenas are best
    suited for temporary buffers and event loop allocators.

    Calling `deallocate_all` will mark all buffers as empty, while calling
    `arena_alloc_free` will free the real memory buffers. This way, you can
    reuse the same arena buffers for multiple loops/frames.

    You can also add unmanaged buffers with `arena_alloc_buffer`. You can
    use this function to add static or stack allocated buffers.

    The allocator returns buffers with `max_align_t` alignment. For stricter
    alignment requirements, you can wrap it with `allocator_aligned.h`.

    For more information on arenas, take a look at:
    https://en.wikipedia.org/wiki/Region-based_memory_management

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

#ifndef ALLOCATOR_ARENA_H
#define ALLOCATOR_ARENA_H

#ifndef ALLOCATOR_H
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_ARENA_ALIGNMENT
    #if !defined(alignof) || !defined(alignas)
        #include <stdalign.h>
    #endif

    #define ALLOCATOR_ARENA_ALIGNMENT alignof(max_align_t)
#endif

#include <stdint.h>
#include <string.h>

struct arena_block {
    size_t used;
    size_t allocated;
    struct arena_block *next;
    void *buffer;
    uint8_t data[];
};

struct arena_alloc {
    struct allocator_t alloc;
    struct arena_block *block;
    allocator_t *base;
};

/* graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2 */
static inline size_t arena_alloc_round(size_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
#if SIZE_MAX > UINT32_MAX
    v |= v >> 32;
#endif
    v++;
    return v;
}

#ifndef ALLOCATOR_ARENA_MIN
    #define ALLOCATOR_ARENA_MIN 4096
#endif

#ifndef ALLOCATOR_ARENA_GROWTH
static inline size_t arena_alloc_growth(size_t prev, size_t req) {
    size_t size = prev * 2 > req ? prev * 2 : req;
    size = arena_alloc_round(size);
    return size > ALLOCATOR_ARENA_MIN ? size : ALLOCATOR_ARENA_MIN;
}
#endif

#define ALLOCATOR_ARENA_ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

static void *arena_alloc_fn(
    allocator_t *self, void *ptr, size_t old, size_t size, size_t zalign
) {
    if (!self || zalign > 0)
        return NULL;

    struct arena_alloc *arena = (struct arena_alloc *)self;
    struct arena_block *block = arena->block, *prev = NULL;
    void *out = NULL;

    if (self == ptr) {
        while (block) {
            block->used = 0;
            block = block->next;
        }
        return NULL;
    }

    /* We need the old size to reallocate. */
    if (ptr && old == 0)
        return NULL;

    if (size > 0) {
        size = ALLOCATOR_ARENA_ALIGN(size, ALLOCATOR_ARENA_ALIGNMENT);

        while (block && block->used + size > block->allocated) {
            prev = block;
            block = block->next;
        }

        if (!block) {
            size_t next = arena_alloc_growth(prev ? prev->allocated : 0, size);
            block = allocate(arena->base, sizeof(struct arena_block) + next);

            if (!block)
                return NULL;

            block->used = 0;
            block->allocated = next;
            block->next = arena->block;
            block->buffer = block->data;
            arena->block = block;
        }

        void *out = (void *)((intptr_t)block->buffer + block->used);
        block->used += size;

        if (ptr) {
            memcpy(out, ptr, old > size ? size : old);

            if (zalign & 1)
                memset((void *)((intptr_t)out + old), 0, size - old);
        } else if (zalign & 1)
            memset(out, 0, size);
    }

    return out;
}

static inline void arena_alloc_init(
    struct arena_alloc *arena, allocator_t *base
) {
    if (arena) {
        allocator_fn **interface = (allocator_fn **)&arena->alloc.interface;
        *interface = &arena_alloc_fn;
        arena->base = base;
        arena->block = NULL;
    }
}

static inline void arena_alloc_buffer(
    struct arena_alloc *arena, void *buffer, size_t size
) {
    if (!arena || !buffer || size == 0)
        return;

    struct arena_block *block;
    block = allocate(arena->base, sizeof(struct arena_block));
    if (block) {
        block->used = 0;
        block->allocated = size;
        block->next = arena->block;
        block->buffer = buffer;
        arena->block = block;
    }
}

static inline void arena_alloc_free(struct arena_alloc *arena) {
    if (arena) {
        struct arena_block *block, *prev;

        while (block) {
            size_t size = sizeof(struct arena_block);
            if (block->buffer == block->data)
                size += prev->allocated;

            prev = block;
            block = block->next;
            deallocate(arena->base, prev, size);
        }
    }
}

#endif
