/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for cJSON (`cJSON_InitHooks`).

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
    `cJSON_Print*` must be released with `cJSON_free`, never with `free`.

        allocator_cjson_init(&my_arena);
        cJSON *json = cJSON_Parse(text);

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

#ifndef ALLOCATOR_CJSON
#define ALLOCATOR_CJSON

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#if defined(__has_include)
    #if __has_include(<cjson/cJSON.h>)
        #include <cjson/cJSON.h>
    #else
        #include <cJSON.h>
    #endif
#else
    #include <cjson/cJSON.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static allocator_t *allocator_cjson_current_ = NULL;

static inline void *CJSON_CDECL allocator_cjson_malloc_fn(size_t size) {
    return allocator_sized_alloc(allocator_cjson_current_, size, 0, 0);
}

static inline void CJSON_CDECL allocator_cjson_free_fn(void *ptr) {
    allocator_sized_free(allocator_cjson_current_, ptr);
}

/** `cJSON_InitHooks` using `alloc`. */
static inline int allocator_cjson_init(allocator_t *alloc) {
    cJSON_Hooks hooks;

    if (!alloc)
        return -1;
    allocator_cjson_current_ = alloc;
    hooks.malloc_fn = &allocator_cjson_malloc_fn;
    hooks.free_fn = &allocator_cjson_free_fn;
    cJSON_InitHooks(&hooks);
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif
