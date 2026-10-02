/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for libuv (`uv_replace_allocator`).

    Library-wide hook: this library only offers process-global memory functions
    without a user-data pointer, so the allocator is kept in a file-local
    variable and shared by every call made through this header.
      - Install it once, from a single translation unit, before the library
        is used for anything else (and before any thread is started).
      - Never switch allocators while blocks from the previous one are alive.
      - The allocator must be thread-safe if the library is used from
        several threads, and must outlive the library.
      - A NULL allocator is reported as an out-of-memory error.

    Must be called before any other libuv function. Returns 0 or a negative
    libuv error code. libuv keeps some memory until `uv_library_shutdown`.

        allocator_libuv_replace(&my_arena);
        uv_loop_init(&loop);

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

#ifndef ALLOCATOR_LIBUV
#define ALLOCATOR_LIBUV

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

static allocator_t *allocator_libuv_current_ = NULL;

static inline void *allocator_libuv_malloc_fn(size_t size) {
    return allocator_sized_alloc(allocator_libuv_current_, size, 0, 0);
}

static inline void *allocator_libuv_realloc_fn(void *ptr, size_t size) {
    return allocator_sized_realloc(allocator_libuv_current_, ptr, size, 0);
}

static inline void *allocator_libuv_calloc_fn(size_t count, size_t size) {
    return allocator_sized_calloc(allocator_libuv_current_, count, size);
}

static inline void allocator_libuv_free_fn(void *ptr) {
    allocator_sized_free(allocator_libuv_current_, ptr);
}

/** `uv_replace_allocator` using `alloc`. */
static inline int allocator_libuv_replace(allocator_t *alloc) {
    allocator_t *previous = allocator_libuv_current_;
    int rc;

    if (!alloc)
        return UV_ENOMEM;
    allocator_libuv_current_ = alloc;
    rc = uv_replace_allocator(
        &allocator_libuv_malloc_fn,
        &allocator_libuv_realloc_fn,
        &allocator_libuv_calloc_fn,
        &allocator_libuv_free_fn
    );
    if (rc != 0)
        allocator_libuv_current_ = previous;
    return rc;
}

#ifdef __cplusplus
}
#endif

#endif
