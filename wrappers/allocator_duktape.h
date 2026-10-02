/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for Duktape.

    Creates a heap whose memory comes from the given allocator, which must
    outlive the heap. Destroy with `duk_destroy_heap`.

        duk_context *ctx = allocator_duk_create_heap(&my_arena, NULL);

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

#ifndef ALLOCATOR_DUKTAPE
#define ALLOCATOR_DUKTAPE

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <duktape.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *allocator_duk_alloc_fn(void *udata, duk_size_t size) {
    return allocator_sized_alloc((allocator_t *)udata, (size_t)size, 0, 0);
}

static inline void *allocator_duk_realloc_fn(
    void *udata, void *ptr, duk_size_t size
) {
    return allocator_sized_realloc((allocator_t *)udata, ptr, (size_t)size, 0);
}

static inline void allocator_duk_free_fn(void *udata, void *ptr) {
    allocator_sized_free((allocator_t *)udata, ptr);
}

/** `duk_create_heap` using `alloc`. `fatal_handler` may be NULL. */
static inline duk_context *allocator_duk_create_heap(
    allocator_t *alloc, duk_fatal_function fatal_handler
) {
    return duk_create_heap(
        &allocator_duk_alloc_fn,
        &allocator_duk_realloc_fn,
        &allocator_duk_free_fn,
        alloc,
        fatal_handler
    );
}

#ifdef __cplusplus
}
#endif

#endif
