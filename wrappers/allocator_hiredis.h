/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for hiredis 1.0+ (`hiredisSetAllocators`).

    Library-wide hook: this library only offers process-global memory functions
    without a user-data pointer, so the allocator is kept in a file-local
    variable and shared by every call made through this header.
      - Install it once, from a single translation unit, before the library
        is used for anything else (and before any thread is started).
      - Never switch allocators while blocks from the previous one are alive.
      - The allocator must be thread-safe if the library is used from
        several threads, and must outlive the library.
      - A NULL allocator is reported as an out-of-memory error.

    Returns 0 on success and -1 for a NULL allocator. The hook also covers
    hiredis' `sds` strings, reply objects and `redisFormatCommand` buffers
    (release those with `redisFreeCommand`). Call `hiredisResetAllocators`
    to restore `malloc` once nothing allocated by the allocator is alive.

        allocator_hiredis_set(&my_arena);
        redisContext *c = redisConnect("127.0.0.1", 6379);

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

#ifndef ALLOCATOR_HIREDIS
#define ALLOCATOR_HIREDIS

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <hiredis/alloc.h>

#ifdef __cplusplus
extern "C" {
#endif

static allocator_t *allocator_hiredis_current_ = NULL;

static inline void *allocator_hiredis_malloc_fn(size_t size) {
    return allocator_sized_alloc(allocator_hiredis_current_, size, 0, 0);
}

static inline void *allocator_hiredis_calloc_fn(size_t nmemb, size_t size) {
    return allocator_sized_calloc(allocator_hiredis_current_, nmemb, size);
}

static inline void *allocator_hiredis_realloc_fn(void *ptr, size_t size) {
    return allocator_sized_realloc(allocator_hiredis_current_, ptr, size, 0);
}

static inline char *allocator_hiredis_strdup_fn(const char *str) {
    return allocator_sized_strdup(allocator_hiredis_current_, str);
}

static inline void allocator_hiredis_free_fn(void *ptr) {
    allocator_sized_free(allocator_hiredis_current_, ptr);
}

/** `hiredisSetAllocators` using `alloc`. */
static inline int allocator_hiredis_set(allocator_t *alloc) {
    hiredisAllocFuncs funcs;

    if (!alloc)
        return -1;
    allocator_hiredis_current_ = alloc;
    funcs.mallocFn = &allocator_hiredis_malloc_fn;
    funcs.callocFn = &allocator_hiredis_calloc_fn;
    funcs.reallocFn = &allocator_hiredis_realloc_fn;
    funcs.strdupFn = &allocator_hiredis_strdup_fn;
    funcs.freeFn = &allocator_hiredis_free_fn;
    hiredisSetAllocators(&funcs);
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif
