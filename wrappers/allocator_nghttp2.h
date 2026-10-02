/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for nghttp2.

    Returns an `nghttp2_mem` for the `nghttp2_session_*_new3` /
    `nghttp2_option_new2`-style constructors. nghttp2 copies the struct; the
    allocator must outlive the session.

        nghttp2_mem mem = allocator_nghttp2_mem(&my_arena);
        nghttp2_session_client_new3(&session, callbacks, user_data, NULL, &mem);

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

#ifndef ALLOCATOR_NGHTTP2
#define ALLOCATOR_NGHTTP2

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <nghttp2/nghttp2.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *allocator_nghttp2_malloc_fn(size_t size, void *data) {
    return allocator_sized_alloc((allocator_t *)data, size, 0, 0);
}

static inline void allocator_nghttp2_free_fn(void *ptr, void *data) {
    allocator_sized_free((allocator_t *)data, ptr);
}

static inline void *allocator_nghttp2_calloc_fn(
    size_t nmemb, size_t size, void *data
) {
    return allocator_sized_calloc((allocator_t *)data, nmemb, size);
}

static inline void *allocator_nghttp2_realloc_fn(
    void *ptr, size_t size, void *data
) {
    return allocator_sized_realloc((allocator_t *)data, ptr, size, 0);
}

/** Memory functions for the `nghttp2_*_new3` constructors. */
static inline nghttp2_mem allocator_nghttp2_mem(allocator_t *alloc) {
    nghttp2_mem mem;
    memset(&mem, 0, sizeof mem);
    mem.mem_user_data = alloc;
    mem.malloc = &allocator_nghttp2_malloc_fn;
    mem.free = &allocator_nghttp2_free_fn;
    mem.calloc = &allocator_nghttp2_calloc_fn;
    mem.realloc = &allocator_nghttp2_realloc_fn;
    return mem;
}

#ifdef __cplusplus
}
#endif

#endif
