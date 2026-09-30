/** `allocator_t` - Interface for custom allocators in C.

    This file provides a wrapper allocator meant for tests: it forwards
    every call to a `base` allocator while counting allocations,
    reallocations and frees, tracking how many blocks and bytes are
    currently live, and catching callers that report the wrong `old`
    size on a free or realloc.

    Setting `budget` to a non-negative number makes the tracker fail
    (return NULL) once that many more allocations would otherwise have
    succeeded, which is useful for exercising a library's out-of-memory
    paths. A negative `budget` (the default after init) never fails.

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

#ifndef ALLOCATOR_TRACKER_H
#define ALLOCATOR_TRACKER_H

#ifndef ALLOCATOR_H
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_TRACKER_ALIGNMENT
    #if !defined(alignof) || !defined(alignas)
        #include <stdalign.h>
    #endif

    #define ALLOCATOR_TRACKER_ALIGNMENT alignof(max_align_t)
#endif

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tracker_allocator_t {
    allocator_t alloc;
    allocator_t *base;

    /* Allocations that may still succeed; negative means unlimited.
       Decremented on every successful allocation or reallocation. */
    long budget;

    size_t allocs;   /* fresh allocations */
    size_t reallocs; /* successful reallocations */
    size_t frees;    /* frees of a non-NULL pointer */
    size_t blocks;   /* live blocks right now */
    size_t bytes;    /* live bytes right now */

    /* Number of times a free or realloc was called with an `old` that
       didn't match the size the block was actually allocated with. */
    size_t badSizes;
} tracker_allocator_t;

/* Every block returned to the caller is preceded by a small fixed-size
   header -- { requested size, distance back to the real allocation } --
   placed immediately before the pointer, regardless of that block's
   alignment. Reading it back never depends on the alignment the caller
   happens to pass on a later call: `deallocate()`/`reallocate()` always
   pass align == 0, even for a block obtained through
   `allocate_aligned()`, so the header's own position can't be a
   function of "whatever alignment this call mentions" or it would be
   unreadable except on the exact call pattern that allocated it. */
struct tracker__header {
    size_t size;
    size_t offset;
};

/** The header sits right before the returned pointer, so the block
    handed to `base` needs to start at least this many bytes earlier --
    enough for the header itself, and enough to satisfy either the
    caller's requested alignment or `ALLOCATOR_TRACKER_ALIGNMENT`,
    whichever is larger. */
static inline size_t tracker__offset(size_t align) {
    size_t offset = align > ALLOCATOR_TRACKER_ALIGNMENT
        ? align
        : ALLOCATOR_TRACKER_ALIGNMENT;

    return offset > sizeof(struct tracker__header)
        ? offset
        : sizeof(struct tracker__header);
}

static void *tracker_allocator_fn(
    allocator_t *self, void *ptr, size_t old, size_t size, size_t zalign
) {
    tracker_allocator_t *t = (tracker_allocator_t *)self;

    if (self == ptr)
        return NULL; /* deallocate_all isn't supported */

    int clear = zalign & 1;
    size_t align = zalign & ~(size_t)1;

    unsigned char *raw = NULL;
    size_t stored = 0;
    size_t offset = 0;

    if (ptr) {
        struct tracker__header header;
        memcpy(&header, (unsigned char *)ptr - sizeof header, sizeof header);
        stored = header.size;
        offset = header.offset;
        raw = (unsigned char *)ptr - offset;

        if (stored != old)
            t->badSizes++;
    }

    if (size == 0) {
        if (raw) {
            deallocate(t->base, raw, stored + offset);
            t->frees++;
            t->blocks--;
            t->bytes -= stored;
        }
        return NULL;
    }

    if (t->budget == 0)
        return NULL;

    /* Keep the block's existing offset unless this call asks for a
       stricter alignment than it already has, in which case a plain
       resize in place can't honor it and the block has to move. */
    size_t wanted = tracker__offset(align);
    size_t use = raw && wanted <= offset ? offset : wanted;
    size_t need = size + use;

    if (need < size)
        return NULL; /* size + use overflowed */

    /* C11 requires aligned_alloc()'s size to be a multiple of its
       alignment; round the real allocation up so `base` can rely on
       that (`use` is always a power of two here). */
    need = (need + use - 1) & ~(use - 1);

    unsigned char *out;

    if (raw && use == offset) {
        out = reallocate_aligned(t->base, raw, stored + offset, need, use);
    } else {
        out = allocate_aligned(t->base, need, use);

        if (out && raw) {
            memcpy(out + use, ptr, stored < size ? stored : size);
            deallocate(t->base, raw, stored + offset);
        }
    }

    if (!out)
        return NULL;

    if (t->budget > 0)
        t->budget--;

    struct tracker__header header = { size, use };
    memcpy(out + use - sizeof header, &header, sizeof header);

    if (raw) {
        t->reallocs++;
        t->bytes += size - stored;
    } else {
        t->allocs++;
        t->blocks++;
        t->bytes += size;
    }

    if (clear) {
        if (raw && size > stored)
            memset(out + use + stored, 0, size - stored);
        else if (!raw)
            memset(out + use, 0, size);
    }

    return out + use;
}

/** Initializes a tracker allocator. `base` is where every allocation
    actually comes from (NULL falls back to `allocator_default`, same
    as any other `allocator_t`). Counters start at zero and `budget`
    starts unlimited (-1); set `out->budget` afterwards to inject
    allocation failures. */
static inline void tracker_allocator_init(
    tracker_allocator_t *out, allocator_t *base
) {
    if (!out)
        return;

    allocator_fn **interface = (allocator_fn **)&out->alloc.interface;
    *interface = &tracker_allocator_fn;
    out->base = base;
    out->budget = -1;
    out->allocs = 0;
    out->reallocs = 0;
    out->frees = 0;
    out->blocks = 0;
    out->bytes = 0;
    out->badSizes = 0;
}

/** Zeroes every counter (including `badSizes`) without touching `base`
    or `budget`, or freeing anything still live. Useful between cases in
    the same test that should each start from a clean count. */
static inline void tracker_allocator_reset(tracker_allocator_t *out) {
    if (!out)
        return;

    out->allocs = 0;
    out->reallocs = 0;
    out->frees = 0;
    out->blocks = 0;
    out->bytes = 0;
    out->badSizes = 0;
}

#ifdef __cplusplus
}
#endif

#endif
