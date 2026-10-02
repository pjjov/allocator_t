/** `allocator_t` - Interface for custom allocators in C.

    Helper used by the third-party library wrappers (`allocator_*.h`).

    Most C libraries free and resize memory without telling the allocator
    how big the block was (`free(ptr)`, `realloc(ptr, size)`), while
    `allocator_t` allocators may need the `old` size to do either.
    This header bridges the two by placing a small header in front of every
    block, recording the size and the offset to the underlying block:

        base                       user pointer (returned)
        |<------- offset --------->|
        [ padding ... | size,offset ][ user data ................ ]

    The offset is `ALLOCATOR_SIZED_ALIGN` (16) bytes by default, or the
    requested alignment if that is larger, so the returned pointer is always
    aligned to at least `ALLOCATOR_SIZED_ALIGN` (assuming the wrapped
    allocator's plain `allocate` returns memory at least that aligned, or
    else to whatever it does guarantee, as the offset is a multiple of 16).

    Semantics (identical to glibc, which most libraries are tested against):
      - `allocator_sized_alloc(a, 0)` returns a valid, unique pointer.
      - `allocator_sized_realloc(a, p, 0)` frees `p` and returns NULL.
      - `allocator_sized_realloc(a, NULL, n)` behaves like alloc.
      - On failure NULL is returned and the old block stays valid.
      - `allocator_sized_free(a, NULL)` does nothing.

    Aligned blocks are requested from the wrapped allocator with
    `allocate_aligned`, so the allocator must implement alignment (or be
    wrapped by `allocator_aligned.h`) if you use a library that asks for
    more than 16 bytes of alignment (e.g. Vulkan).

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
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ALLOCATOR_SIZED_ALIGN
    #define ALLOCATOR_SIZED_ALIGN ((size_t)16)
#endif

#define ALLOCATOR_SIZED_MAX_ ((size_t)-1)

typedef struct allocator_sized_header {
    size_t size;   /* size requested by the library */
    size_t offset; /* distance from the underlying block to the user pointer */
} allocator_sized_header;

/* The header has to fit in front of the user pointer. */
typedef char allocator_sized_header_fits_
    [sizeof(allocator_sized_header) <= ALLOCATOR_SIZED_ALIGN ? 1 : -1];

static inline allocator_sized_header *allocator_sized_header_(void *ptr) {
    return (allocator_sized_header *)((unsigned char *)ptr
                                      - sizeof(allocator_sized_header));
}

/* Size of the underlying block for a user size. Blocks with extended
   alignment are rounded up to a multiple of it, because allocators built on
   C11 `aligned_alloc` (like allocator_std.h) require that. 0 on overflow. */
static inline size_t allocator_sized_total_(size_t offset, size_t size) {
    size_t total;
    if (size > ALLOCATOR_SIZED_MAX_ - offset)
        return 0;
    total = offset + size;
    if (offset > ALLOCATOR_SIZED_ALIGN) {
        size_t rest = total & (offset - 1);
        if (rest) {
            if (offset - rest > ALLOCATOR_SIZED_MAX_ - total)
                return 0;
            total += offset - rest;
        }
    }
    return total;
}

static inline void *allocator_sized_finish_(
    unsigned char *base, size_t offset, size_t size
) {
    unsigned char *user = base + offset;
    allocator_sized_header *header = allocator_sized_header_(user);
    header->size = size;
    header->offset = offset;
    return user;
}

/** Size that was requested for the block `ptr` (0 for NULL). */
static inline size_t allocator_sized_size(const void *ptr) {
    return ptr ? allocator_sized_header_((void *)ptr)->size : 0;
}

/** Allocates `size` bytes aligned to `align` (0 = default), optionally zeroed.
    `align` has to be a power of 2 if it is larger than the default. */
static inline void *allocator_sized_alloc(
    allocator_t *alloc, size_t size, size_t align, int zero
) {
    size_t offset = ALLOCATOR_SIZED_ALIGN;
    size_t total;
    unsigned char *base;

    if (align > offset) {
        if (align & (align - 1))
            return NULL;
        offset = align;
    }
    total = allocator_sized_total_(offset, size);
    if (!total)
        return NULL;

    if (offset > ALLOCATOR_SIZED_ALIGN) {
        base = (unsigned char *)(zero ? zallocate_aligned(alloc, total, offset)
                                      : allocate_aligned(alloc, total, offset));
    } else {
        base = (unsigned char *)(zero ? zallocate(alloc, total)
                                      : allocate(alloc, total));
    }
    return base ? allocator_sized_finish_(base, offset, size) : NULL;
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
        alloc,
        (unsigned char *)ptr - header.offset,
        allocator_sized_total_(header.offset, header.size)
    );
}

/** `realloc`. The alignment of the original block is preserved.
    When `zero` is set, the bytes added by growing are zeroed. */
static inline void *allocator_sized_realloc(
    allocator_t *alloc, void *ptr, size_t size, int zero
) {
    allocator_sized_header header;
    unsigned char *base, *moved;
    size_t old_total, new_total;

    if (!ptr)
        return allocator_sized_alloc(alloc, size, 0, zero);
    if (size == 0) {
        allocator_sized_free(alloc, ptr);
        return NULL;
    }

    header = *allocator_sized_header_(ptr);
    new_total = allocator_sized_total_(header.offset, size);
    if (!new_total)
        return NULL;
    old_total = allocator_sized_total_(header.offset, header.size);
    base = (unsigned char *)ptr - header.offset;

    if (header.offset > ALLOCATOR_SIZED_ALIGN) {
        moved = (unsigned char *)(zero ? zreallocate_aligned(
                                             alloc,
                                             base,
                                             old_total,
                                             new_total,
                                             header.offset
                                         )
                                       : reallocate_aligned(
                                             alloc,
                                             base,
                                             old_total,
                                             new_total,
                                             header.offset
                                         ));
    } else {
        moved = (unsigned char
                     *)(zero ? zreallocate(alloc, base, old_total, new_total)
                             : reallocate(alloc, base, old_total, new_total));
    }
    return moved ? allocator_sized_finish_(moved, header.offset, size) : NULL;
}

/** `strdup` using the allocator. */
static inline char *allocator_sized_strdup(
    allocator_t *alloc, const char *str
) {
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
