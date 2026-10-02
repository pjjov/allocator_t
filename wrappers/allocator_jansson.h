/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for Jansson (`json_set_alloc_funcs`).

    Library-wide hook: this library only offers process-global memory functions
    without a user-data pointer, so the allocator is kept in a file-local
    variable and shared by every call made through this header.
      - Install it once, from a single translation unit, before the library
        is used for anything else (and before any thread is started).
      - Never switch allocators while blocks from the previous one are alive.
      - The allocator must be thread-safe if the library is used from
        several threads, and must outlive the library.
      - A NULL allocator is reported as an out-of-memory error.

    Returns 0 on success and -1 for a NULL allocator. Strings returned by
    `json_dumps` and friends must be released with `allocator_jansson_free`
    (or the free function from `json_get_alloc_funcs`), never with `free`.

        allocator_jansson_set(&my_arena);
        json_t *root = json_loads(text, 0, &error);

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

#ifndef ALLOCATOR_JANSSON
#define ALLOCATOR_JANSSON

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <jansson.h>

#ifdef __cplusplus
extern "C" {
#endif

static allocator_t *allocator_jansson_current_ = NULL;

static inline void *allocator_jansson_malloc_fn(size_t size) {
    return allocator_sized_alloc(allocator_jansson_current_, size, 0, 0);
}

static inline void allocator_jansson_free(void *ptr) {
    allocator_sized_free(allocator_jansson_current_, ptr);
}

/** `json_set_alloc_funcs` using `alloc`. */
static inline int allocator_jansson_set(allocator_t *alloc) {
    if (!alloc)
        return -1;
    allocator_jansson_current_ = alloc;
    json_set_alloc_funcs(&allocator_jansson_malloc_fn, &allocator_jansson_free);
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif
