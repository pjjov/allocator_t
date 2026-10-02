/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for liblzma (xz).

    Fills a `lzma_allocator`, which can be assigned to `lzma_stream.allocator`
    before initializing the coder, or passed to the buffer functions
    (`lzma_stream_buffer_encode`, `lzma_stream_buffer_decode`, ...).
    Both the `lzma_allocator` struct and the allocator must outlive the stream,
    because liblzma keeps the pointer instead of copying the struct.

        lzma_allocator la;
        allocator_liblzma_init(&la, &my_arena);
        lzma_stream strm = LZMA_STREAM_INIT;
        strm.allocator = &la;
        lzma_easy_encoder(&strm, 6, LZMA_CHECK_CRC64);

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

#ifndef ALLOCATOR_LIBLZMA
#define ALLOCATOR_LIBLZMA

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <lzma.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *LZMA_API_CALL
allocator_liblzma_alloc_fn(void *opaque, size_t nmemb, size_t size) {
    if (size && nmemb > (size_t)-1 / size)
        return NULL;
    return allocator_sized_alloc((allocator_t *)opaque, nmemb * size, 0, 0);
}

static inline void LZMA_API_CALL
allocator_liblzma_free_fn(void *opaque, void *ptr) {
    allocator_sized_free((allocator_t *)opaque, ptr);
}

/** Fills `out` so that liblzma uses `alloc`. */
static inline void allocator_liblzma_init(
    lzma_allocator *out, allocator_t *alloc
) {
    out->alloc = &allocator_liblzma_alloc_fn;
    out->free = &allocator_liblzma_free_fn;
    out->opaque = alloc;
}

#ifdef __cplusplus
}
#endif

#endif
