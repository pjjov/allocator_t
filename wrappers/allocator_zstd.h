/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for Zstandard.

    Returns a `ZSTD_customMem` for the `*_advanced` context constructors.
    This is part of zstd's "static linking only" API: the wrapper defines
    `ZSTD_STATIC_LINKING_ONLY` before including <zstd.h>, so link zstd
    statically or make sure the shared library's version matches the header.

        ZSTD_customMem mem = allocator_zstd_mem(&my_arena);
        ZSTD_CCtx *cctx = ZSTD_createCCtx_advanced(mem);
        ...
        ZSTD_freeCCtx(cctx);   // frees with the same allocator

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

#ifndef ALLOCATOR_ZSTD
#define ALLOCATOR_ZSTD

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#ifndef ZSTD_STATIC_LINKING_ONLY
    #define ZSTD_STATIC_LINKING_ONLY
#endif
#include <zstd.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *allocator_zstd_alloc_fn(void *opaque, size_t size) {
    return allocator_sized_alloc((allocator_t *)opaque, size, 0, 0);
}

static inline void allocator_zstd_free_fn(void *opaque, void *address) {
    allocator_sized_free((allocator_t *)opaque, address);
}

/** Memory functions for `ZSTD_create*_advanced`. */
static inline ZSTD_customMem allocator_zstd_mem(allocator_t *alloc) {
    ZSTD_customMem mem;
    mem.customAlloc = &allocator_zstd_alloc_fn;
    mem.customFree = &allocator_zstd_free_fn;
    mem.opaque = alloc;
    return mem;
}

#ifdef __cplusplus
}
#endif

#endif
