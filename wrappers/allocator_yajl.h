/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for YAJL.

    Returns a `yajl_alloc_funcs` for `yajl_alloc` (parser) and `yajl_gen_alloc`
    (generator). YAJL copies the struct, so it can be a temporary; the allocator
    must outlive the parser/generator.

        yajl_alloc_funcs af = allocator_yajl_funcs(&my_arena);
        yajl_gen gen = yajl_gen_alloc(&af);

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

#ifndef ALLOCATOR_YAJL
#define ALLOCATOR_YAJL

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <yajl/yajl_common.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *allocator_yajl_malloc_fn(void *ctx, size_t size) {
    return allocator_sized_alloc((allocator_t *)ctx, size, 0, 0);
}

static inline void *allocator_yajl_realloc_fn(
    void *ctx, void *ptr, size_t size
) {
    return allocator_sized_realloc((allocator_t *)ctx, ptr, size, 0);
}

static inline void allocator_yajl_free_fn(void *ctx, void *ptr) {
    allocator_sized_free((allocator_t *)ctx, ptr);
}

/** Allocation functions for `yajl_alloc` and `yajl_gen_alloc`. */
static inline yajl_alloc_funcs allocator_yajl_funcs(allocator_t *alloc) {
    yajl_alloc_funcs funcs;
    funcs.malloc = &allocator_yajl_malloc_fn;
    funcs.realloc = &allocator_yajl_realloc_fn;
    funcs.free = &allocator_yajl_free_fn;
    funcs.ctx = alloc;
    return funcs;
}

#ifdef __cplusplus
}
#endif

#endif
