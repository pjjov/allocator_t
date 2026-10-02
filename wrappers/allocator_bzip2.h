/** `allocator_t` - Interface for custom allocators in C.

    `allocator_t` wrapper for bzip2 (libbz2).

    `bz_stream` carries `bzalloc`, `bzfree` and `opaque`. Call
    `allocator_bzip2_init` on the stream before `BZ2_bzCompressInit` or
    `BZ2_bzDecompressInit`. The allocator must outlive the stream.

        bz_stream strm = {0};
        allocator_bzip2_init(&strm, &my_arena);
        BZ2_bzCompressInit(&strm, 9, 0, 0);

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

#ifndef ALLOCATOR_BZIP2
#define ALLOCATOR_BZIP2

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <bzlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *allocator_bzip2_alloc_fn(
    void *opaque, int items, int size
) {
    size_t bytes;
    if (items < 0 || size < 0)
        return NULL;
    bytes = (size_t)items * (size_t)size;
    if (size && bytes / (size_t)size != (size_t)items)
        return NULL;
    return allocator_sized_alloc((allocator_t *)opaque, bytes, 0, 0);
}

static inline void allocator_bzip2_free_fn(void *opaque, void *address) {
    allocator_sized_free((allocator_t *)opaque, address);
}

/** Makes the stream use `alloc`. Call before `BZ2_bz*Init`. */
static inline void allocator_bzip2_init(bz_stream *strm, allocator_t *alloc) {
    strm->bzalloc = &allocator_bzip2_alloc_fn;
    strm->bzfree = &allocator_bzip2_free_fn;
    strm->opaque = alloc;
}

#ifdef __cplusplus
}
#endif

#endif