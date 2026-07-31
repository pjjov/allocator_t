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

#ifndef ALLOCATOR_H
    #include "allocator.h"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

struct join_alloc {
    size_t size;
    size_t align;

    size_t offset;
    void *buffer;
};

static size_t join_alloc_align_up(size_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

static inline void *allocate_joined(struct join_alloc *blocks, size_t count) {
    if (!blocks || count == 0)
        return NULL;

    struct join_alloc *b, *end = &blocks[count];
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

    if (!(raw = malloc(total)))
        return NULL;

    uintptr_t base = join_alloc_align_up((uintptr_t)raw, max_align);

    for (b = blocks; b < end; b++)
        b->buffer = (void *)(base + b->offset);

    return raw;
}

#endif