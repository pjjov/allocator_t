/** `allocator_t` - Interface for custom allocators in C.

    This files provides the `allocate_joined` function which groups multiple
    allocation with different size and alignment requirements into a single
    call to the allocator.

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

#ifndef ALLOCATOR_JOINED_H
#define ALLOCATOR_JOINED_H

#ifndef ALLOCATOR_API
    #define ALLOCATOR_API static inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ALLOCATOR_JOINED_TYPEOF
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
        #define ALLOCATOR_JOINED_TYPEOF(x) typeof(x)
    #else
        #define ALLOCATOR_JOINED_TYPEOF(x) __typeof__(x)
    #endif
#endif

#ifndef ALLOCATOR_H
    #include "allocator.h"
#endif

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct joined_allocation_t {
    size_t size;
    size_t align;

    size_t offset;
    void *buffer;

    void *out; /* optional, address of a pointer that receives `buffer` */
} joined_allocation_t;

static size_t join_alloc_align_up(size_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

ALLOCATOR_API void *allocate_joined(
    allocator_t *alloc, joined_allocation_t *blocks, size_t count, size_t *size
) {
    if (!blocks || !size || count == 0)
        return NULL;

    joined_allocation_t *b, *end = &blocks[count];
    size_t max_align = alignof(max_align_t);

    for (b = blocks; b < end; b++) {
        if (b->align == 0 || (b->align & (b->align - 1)) != 0)
            return NULL; /* alignment must be power of two */

        if (b->align > max_align)
            max_align = b->align;
    }

    size_t offset = 0;

    for (b = blocks; b < end; b++) {
        offset = join_alloc_align_up(offset, b->align);
        b->offset = offset;
        offset += b->size;
    }

    /* Allocate extra space so we can align the base pointer. */
    size_t total = offset + max_align - 1;
    void *raw;

    if (!(raw = allocate(alloc, total)))
        return NULL;

    uintptr_t base = join_alloc_align_up((uintptr_t)raw, max_align);

    for (b = blocks; b < end; b++) {
        b->buffer = (void *)(base + b->offset);

        if (b->out)
            memcpy(b->out, &b->buffer, sizeof(void *));
    }

    *size = total;
    return raw;
}

/* Describes an array of `n` objects for `ALLOCATE_JOINED`, where `p` is the
   address of the pointer that will receive the block. */
#define JOINED_ARRAY(p, n)                              \
    { .size = (size_t)(n) * sizeof(**(p)),              \
      .align = alignof(ALLOCATOR_JOINED_TYPEOF(**(p))), \
      .out = (p) }

/* Describes a single object for `ALLOCATE_JOINED`. */
#define JOINED_BLOCK(p) JOINED_ARRAY(p, 1)

/* Allocates all `JOINED_BLOCK`s and `JOINED_ARRAY`s in a single allocation
   and points each of them at its block. Evaluates to the raw buffer to
   deallocate, or NULL on failure (the pointers are left untouched).

       int *a;
       struct example *b;
       size_t size;
       void *buffer = ALLOCATE_JOINED(
           alloc, &size, JOINED_ARRAY(&a, 16), JOINED_BLOCK(&b)
       );
*/
#define ALLOCATE_JOINED(alloc, size, ...)               \
    allocate_joined(                                    \
        (alloc),                                        \
        (joined_allocation_t[]) { __VA_ARGS__ },        \
        sizeof((joined_allocation_t[]) { __VA_ARGS__ }) \
            / sizeof(joined_allocation_t),              \
        (size)                                          \
    )

#ifdef __cplusplus
}
#endif

#endif