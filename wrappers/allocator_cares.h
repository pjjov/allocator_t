/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for c-ares (`ares_library_init_mem`).

    Library-wide hook: this library only offers process-global memory functions
    without a user-data pointer, so the allocator is kept in a file-local
    variable and shared by every call made through this header.
      - Install it once, from a single translation unit, before the library
        is used for anything else (and before any thread is started).
      - Never switch allocators while blocks from the previous one are alive.
      - The allocator must be thread-safe if the library is used from
        several threads, and must outlive the library.
      - A NULL allocator is reported as an out-of-memory error.

    Use `allocator_cares_library_init` instead of `ares_library_init`, and
    finish with `ares_library_cleanup`. Returns an `ARES_*` status code.

        allocator_cares_library_init(ARES_LIB_INIT_ALL, &my_arena);
        ares_init(&channel);

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

#ifndef ALLOCATOR_CARES
#define ALLOCATOR_CARES

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <ares.h>

#ifdef __cplusplus
extern "C" {
#endif

static allocator_t *allocator_cares_current_ = NULL;

static inline void *allocator_cares_malloc_fn(size_t size) {
    return allocator_sized_alloc(allocator_cares_current_, size, 0, 0);
}

static inline void allocator_cares_free_fn(void *ptr) {
    allocator_sized_free(allocator_cares_current_, ptr);
}

static inline void *allocator_cares_realloc_fn(void *ptr, size_t size) {
    return allocator_sized_realloc(allocator_cares_current_, ptr, size, 0);
}

/** `ares_library_init_mem` using `alloc`. */
static inline int allocator_cares_library_init(int flags, allocator_t *alloc) {
    allocator_t *previous = allocator_cares_current_;
    int rc;

    if (!alloc)
        return ARES_ENOMEM;
    allocator_cares_current_ = alloc;
    rc = ares_library_init_mem(
        flags,
        &allocator_cares_malloc_fn,
        &allocator_cares_free_fn,
        &allocator_cares_realloc_fn
    );
    if (rc != ARES_SUCCESS)
        allocator_cares_current_ = previous;
    return rc;
}

#ifdef __cplusplus
}
#endif

#endif
