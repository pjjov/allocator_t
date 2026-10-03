/** `allocator_t` - Interface for custom allocators in C.

    Size and alignment utility for allocator_t allocators.

    Plain functions (no wrapper allocator) that give any `allocator_t`
    `malloc`-style semantics: `free(ptr)` and `realloc(ptr, size)` without
    knowing the old size, plus aligned allocations even on allocators that do
    not implement alignment. It is used by the third-party library wrappers
    (`allocator_*.h`) and can be used directly.

    How it works: every block gets a small header right in front of the
    pointer that is handed out. It records the size, the distance to the real
    start of the block and how the block was made:

        base                        user pointer (returned)
        |<-------- offset --------->|
        [ ........... | header (16) ][ user data .................. ]

    There are three kinds of blocks, picked per allocation:

      plain     alignment <= 16. One plain `allocate` call, 16 bytes of
                overhead. Plain allocations must be aligned to at least what
                the allocator promises for `malloc`-like use.
      native    alignment > 16, served by the allocator's own
                `allocate_aligned`. The block is `alignment + size` bytes
                (rounded up to a multiple of the alignment, as C11
                `aligned_alloc` requires). `reallocate` keeps it in place.
      emulated  alignment > 16 on an allocator without alignment support.
                The block is over-allocated by `alignment + 16` bytes with a
                plain `allocate`, and the pointer is rounded up inside it.
                `realloc` allocates, copies and frees, since the alignment of
                a moved block cannot be trusted.

    Which one is used for over-aligned requests is chosen with
    `ALLOCATOR_SIZED_POLICY`, defined before including this file:

      ALLOCATOR_SIZED_POLICY_AUTO     (default) try native; if the allocator
                                      returns NULL or a misaligned pointer,
                                      emulate. Costs one extra failed call
                                      per allocation on allocators without
                                      alignment, and a native attempt that
                                      failed from out-of-memory is retried.
      ALLOCATOR_SIZED_POLICY_NATIVE   native only; NULL if unsupported.
      ALLOCATOR_SIZED_POLICY_EMULATE  never ask for alignment: use this for
                                      allocators known to lack it.

    Native blocks require `reallocate_aligned` to preserve the alignment, as
    the interface demands of an allocator that supports alignment at all.

    Semantics (identical to glibc, which most libraries are tested against):
      - `allocator_sized_alloc(a, 0, ...)` returns a valid, unique pointer.
      - `allocator_sized_realloc(a, p, 0, ...)` frees `p` and returns NULL.
      - `allocator_sized_realloc(a, NULL, n, ...)` behaves like alloc.
      - On failure NULL is returned and the old block stays valid.
      - `allocator_sized_free(a, NULL)` does nothing.
      - Alignments must be powers of 2, at most 2^30.

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

#ifndef ALLOCATOR_SIZED
#define ALLOCATOR_SIZED

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Alignment of plain blocks, and size of the header. */
#ifndef ALLOCATOR_SIZED_ALIGN
    #define ALLOCATOR_SIZED_ALIGN ((size_t)16)
#endif

#define ALLOCATOR_SIZED_POLICY_AUTO 0
#define ALLOCATOR_SIZED_POLICY_NATIVE 1
#define ALLOCATOR_SIZED_POLICY_EMULATE 2
#ifndef ALLOCATOR_SIZED_POLICY
    #define ALLOCATOR_SIZED_POLICY ALLOCATOR_SIZED_POLICY_AUTO
#endif

#define ALLOCATOR_SIZED_MAX_ ((size_t) - 1)
#define ALLOCATOR_SIZED_MAX_ALIGN_ ((size_t)1 << 30)

#define ALLOCATOR_SIZED_PLAIN_ 0
#define ALLOCATOR_SIZED_NATIVE_ 1
#define ALLOCATOR_SIZED_EMULATED_ 2

typedef struct allocator_sized_header {
    size_t size;          /* size requested by the caller */
    uint32_t offset;      /* distance from the underlying block to the pointer */
    unsigned char log2;   /* log2 of the alignment of the pointer */
    unsigned char mode;   /* ALLOCATOR_SIZED_{PLAIN,NATIVE,EMULATED}_ */
} allocator_sized_header;

/* The header has to fit in front of the pointer. */
typedef char allocator_sized_header_fits_
    [sizeof(allocator_sized_header) <= ALLOCATOR_SIZED_ALIGN ? 1 : -1];

static inline allocator_sized_header *allocator_sized_header_(void *ptr) {
    return (allocator_sized_header *)((unsigned char *)ptr
                                      - sizeof(allocator_sized_header));
}

static inline unsigned char allocator_sized_log2_(size_t align) {
    unsigned char n = 0;
    while (align > 1) {
        align >>= 1;
        n++;
    }
    return n;
}

/* Size of the underlying block for a user size. 0 on overflow. */
static inline size_t allocator_sized_total_(
    unsigned char mode, size_t align, size_t size
) {
    size_t total, rest;

    if (mode == ALLOCATOR_SIZED_PLAIN_) {
        if (size > ALLOCATOR_SIZED_MAX_ - ALLOCATOR_SIZED_ALIGN)
            return 0;
        return ALLOCATOR_SIZED_ALIGN + size;
    }
    if (mode == ALLOCATOR_SIZED_NATIVE_) {
        if (size > ALLOCATOR_SIZED_MAX_ - align)
            return 0;
        total = align + size;
        rest = total & (align - 1);
        if (rest) {
            if (align - rest > ALLOCATOR_SIZED_MAX_ - total)
                return 0;
            total += align - rest;
        }
        return total;
    }
    if (size > ALLOCATOR_SIZED_MAX_ - align - ALLOCATOR_SIZED_ALIGN)
        return 0;
    return size + align + ALLOCATOR_SIZED_ALIGN;
}

static inline void *allocator_sized_finish_(
    unsigned char *base, unsigned char *user, size_t size,
    unsigned char log2, unsigned char mode
) {
    allocator_sized_header *header = allocator_sized_header_(user);
    header->size = size;
    header->offset = (uint32_t)(user - base);
    header->log2 = log2;
    header->mode = mode;
    return user;
}

static inline void *allocator_sized_alloc_plain_(
    allocator_t *alloc, size_t size, int zero
) {
    size_t total = allocator_sized_total_(ALLOCATOR_SIZED_PLAIN_, 0, size);
    unsigned char *base;
    if (!total)
        return NULL;
    base = (unsigned char *)(zero ? zallocate(alloc, total)
                                  : allocate(alloc, total));
    if (!base)
        return NULL;
    return allocator_sized_finish_(
        base, base + ALLOCATOR_SIZED_ALIGN, size,
        allocator_sized_log2_(ALLOCATOR_SIZED_ALIGN), ALLOCATOR_SIZED_PLAIN_);
}

#if ALLOCATOR_SIZED_POLICY != ALLOCATOR_SIZED_POLICY_EMULATE
static inline void *allocator_sized_alloc_native_(
    allocator_t *alloc, size_t size, size_t align, int zero
) {
    size_t total = allocator_sized_total_(ALLOCATOR_SIZED_NATIVE_, align, size);
    unsigned char *base;
    if (!total)
        return NULL;
    base = (unsigned char *)(zero ? zallocate_aligned(alloc, total, align)
                                  : allocate_aligned(alloc, total, align));
    if (!base)
        return NULL;
    if ((uintptr_t)base & (align - 1)) { /* alignment silently ignored */
        deallocate(alloc, base, total);
        return NULL;
    }
    return allocator_sized_finish_(
        base, base + align, size, allocator_sized_log2_(align),
        ALLOCATOR_SIZED_NATIVE_);
}
#endif

static inline void *allocator_sized_alloc_emulated_(
    allocator_t *alloc, size_t size, size_t align, int zero
) {
    size_t total = allocator_sized_total_(ALLOCATOR_SIZED_EMULATED_, align, size);
    unsigned char *base;
    uintptr_t addr;
    if (!total)
        return NULL;
    base = (unsigned char *)(zero ? zallocate(alloc, total)
                                  : allocate(alloc, total));
    if (!base)
        return NULL;
    addr = ((uintptr_t)base + ALLOCATOR_SIZED_ALIGN + align - 1)
           & ~((uintptr_t)align - 1);
    return allocator_sized_finish_(
        base, base + (addr - (uintptr_t)base), size,
        allocator_sized_log2_(align), ALLOCATOR_SIZED_EMULATED_);
}

/** Size that was requested for the block `ptr` (0 for NULL). */
static inline size_t allocator_sized_size(const void *ptr) {
    return ptr ? allocator_sized_header_((void *)ptr)->size : 0;
}

/** Alignment the block `ptr` is guaranteed to have (0 for NULL). */
static inline size_t allocator_sized_align(const void *ptr) {
    return ptr ? (size_t)1 << allocator_sized_header_((void *)ptr)->log2 : 0;
}

/** Allocates `size` bytes aligned to `align` (0 = default), optionally zeroed.
    `align` has to be a power of 2. */
static inline void *allocator_sized_alloc(
    allocator_t *alloc, size_t size, size_t align, int zero
) {
    void *out;

    if (align & (align - 1))
        return NULL;
    if (align <= ALLOCATOR_SIZED_ALIGN)
        return allocator_sized_alloc_plain_(alloc, size, zero);
    if (align > ALLOCATOR_SIZED_MAX_ALIGN_)
        return NULL;

#if ALLOCATOR_SIZED_POLICY == ALLOCATOR_SIZED_POLICY_EMULATE
    out = allocator_sized_alloc_emulated_(alloc, size, align, zero);
#else
    out = allocator_sized_alloc_native_(alloc, size, align, zero);
    #if ALLOCATOR_SIZED_POLICY == ALLOCATOR_SIZED_POLICY_AUTO
    if (!out)
        out = allocator_sized_alloc_emulated_(alloc, size, align, zero);
    #endif
#endif
    return out;
}

/** `calloc` with overflow checking. */
static inline void *allocator_sized_calloc(
    allocator_t *alloc, size_t count, size_t size
) {
    if (size && count > ALLOCATOR_SIZED_MAX_ / size)
        return NULL;
    return allocator_sized_alloc(alloc, count * size, 0, 1);
}

/** Frees a block from this header. `NULL` is ignored. */
static inline void allocator_sized_free(allocator_t *alloc, void *ptr) {
    allocator_sized_header header;
    if (!ptr)
        return;
    header = *allocator_sized_header_(ptr);
    deallocate(
        alloc, (unsigned char *)ptr - header.offset,
        allocator_sized_total_(
            header.mode, (size_t)1 << header.log2, header.size));
}

/** `realloc`. The alignment of the original block is preserved.
    When `zero` is set, the bytes added by growing are zeroed. */
static inline void *allocator_sized_realloc(
    allocator_t *alloc, void *ptr, size_t size, int zero
) {
    allocator_sized_header header;
    size_t align, old_total, new_total;
    unsigned char *base, *moved;
    void *fresh;

    if (!ptr)
        return allocator_sized_alloc(alloc, size, 0, zero);
    if (size == 0) {
        allocator_sized_free(alloc, ptr);
        return NULL;
    }

    header = *allocator_sized_header_(ptr);
    align = (size_t)1 << header.log2;
    new_total = allocator_sized_total_(header.mode, align, size);
    if (!new_total)
        return NULL;

    if (header.mode == ALLOCATOR_SIZED_EMULATED_) {
        fresh = allocator_sized_alloc_emulated_(alloc, size, align, zero);
        if (!fresh)
            return NULL;
        memcpy(fresh, ptr, header.size < size ? header.size : size);
        allocator_sized_free(alloc, ptr);
        return fresh;
    }

    old_total = allocator_sized_total_(header.mode, align, header.size);
    base = (unsigned char *)ptr - header.offset;
    if (header.mode == ALLOCATOR_SIZED_NATIVE_) {
        moved = (unsigned char *)(zero
            ? zreallocate_aligned(alloc, base, old_total, new_total, align)
            : reallocate_aligned(alloc, base, old_total, new_total, align));
    } else {
        moved = (unsigned char *)(zero
            ? zreallocate(alloc, base, old_total, new_total)
            : reallocate(alloc, base, old_total, new_total));
    }
    if (!moved)
        return NULL;
    return allocator_sized_finish_(
        moved, moved + header.offset, size, header.log2, header.mode);
}

/** `strdup` using the allocator. */
static inline char *allocator_sized_strdup(allocator_t *alloc, const char *str) {
    size_t len;
    char *out;
    if (!str)
        return NULL;
    len = strlen(str) + 1;
    out = (char *)allocator_sized_alloc(alloc, len, 0, 0);
    if (out)
        memcpy(out, str, len);
    return out;
}

#ifdef __cplusplus
}
#endif

#endif
