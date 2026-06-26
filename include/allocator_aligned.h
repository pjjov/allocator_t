/** `allocator_t` - Interface for custom allocators in C.

    This wrapper enables handling of custom address alignments
    for allocators that don't support it themselves.

    Alongide that, you can set a minimal alignment for all
    allocations (for performance and vectorized instructions).

    The wrapper cannot check if the underlying allocator is capable
    of aligned allocations, resulting in perhaps unnecessary overhead.
    The memory overhead is equal to `sizeof(void *) + alignment - 1`.

    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0

    Copyright 2025 Predrag Jovanović

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

#ifndef ALLOCATOR_ALIGNED
#define ALLOCATOR_ALIGNED

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#include <stdint.h>
#include <string.h>

struct align_alloc {
    allocator_t alloc;
    allocator_t *base;
    size_t min;
};

static void *align_alloc_fn(
    allocator_t *self, void *ptr, size_t old, size_t size, size_t zalign
) {
    if (!self || (ptr && size > 0 && old == 0))
        return NULL;

    struct align_alloc *alloc = (struct align_alloc *)self;

    if (self == ptr) {
        deallocate_all(alloc->base);
        return NULL;
    }

    size_t align = (zalign & ~1);
    if (align < alloc->min)
        align = alloc->min;

    /* https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2 */
    align--;
    align |= align >> 1;
    align |= align >> 2;
    align |= align >> 4;
    align |= align >> 8;
    align |= align >> 16;
    align++;

    void *out = NULL;
    if (size > 0) {
        size_t realSize = size + sizeof(void *) + (align - 1);
        void *buf = allocate(alloc->base, realSize);
        if (!buf)
            return NULL;

        uintptr_t aligned = ((uintptr_t)buf + sizeof(void *) + (align - 1))
            & (~(align - 1));
        void **header = (void **)(aligned - sizeof(void *));

        *header = buf;
        out = (void *)aligned;
    } else {
        if (ptr) {
            void *real = *(void **)((uintptr_t)ptr - sizeof(void *));
            deallocate(
                alloc->base,
                real,
                old + sizeof(void *) + ((uintptr_t)real - (uintptr_t)ptr)
            );
        }
        return NULL;
    }

    if (ptr) {
        memcpy(out, ptr, old < size ? old : size);

        void *real = *(void **)((uintptr_t)ptr - sizeof(void *));
        deallocate(
            alloc->base,
            real,
            old + sizeof(void *) + ((uintptr_t)real - (uintptr_t)ptr)
        );

        if (zalign & 1) {
            memset(&((unsigned char *)out)[old], 0, size - old);
        }
    } else if (zalign & 1) {
        memset(out, 0, size);
    }

    return out;
}

static inline void align_alloc_init(
    struct align_alloc *out, allocator_t *base, size_t min
) {
    if (out) {
        allocator_fn **interface = (allocator_fn **)&out->alloc.interface;
        *interface = &align_alloc_fn;
        out->base = base;
        out->min = min ? min : sizeof(max_align_t);
    }
}

#endif
