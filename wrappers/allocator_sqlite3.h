/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for SQLite (`SQLITE_CONFIG_MALLOC`).

    Library-wide hook: this library only offers process-global memory functions
    without a user-data pointer, so the allocator is kept in a file-local
    variable and shared by every call made through this header.
      - Install it once, from a single translation unit, before the library
        is used for anything else (and before any thread is started).
      - Never switch allocators while blocks from the previous one are alive.
      - The allocator must be thread-safe if the library is used from
        several threads, and must outlive the library.
      - A NULL allocator is reported as an out-of-memory error.

    Call `allocator_sqlite3_config` before `sqlite3_initialize` (or after
    `sqlite3_shutdown`); SQLite answers `SQLITE_MISUSE` otherwise. Sizes are
    rounded up to a multiple of 8, as SQLite expects from `xRoundup`.
    Memory from `sqlite3_malloc` and friends (e.g. `sqlite3_mprintf` results)
    is also served by the allocator.

        allocator_sqlite3_config(&my_arena);
        sqlite3_open(":memory:", &db);

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

#ifndef ALLOCATOR_SQLITE3
#define ALLOCATOR_SQLITE3

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <sqlite3.h>

#ifdef __cplusplus
extern "C" {
#endif

static allocator_t *allocator_sqlite3_current_ = NULL;

static inline int allocator_sqlite3_round_(int n) {
    return (int)(((unsigned)n + 7u) & ~7u);
}

static inline void *allocator_sqlite3_malloc_fn(int n) {
    if (n <= 0)
        return NULL;
    return allocator_sized_alloc(
        allocator_sqlite3_current_, (size_t)allocator_sqlite3_round_(n), 0, 0
    );
}

static inline void allocator_sqlite3_free_fn(void *ptr) {
    allocator_sized_free(allocator_sqlite3_current_, ptr);
}

static inline void *allocator_sqlite3_realloc_fn(void *ptr, int n) {
    if (n <= 0)
        return NULL;
    return allocator_sized_realloc(
        allocator_sqlite3_current_, ptr, (size_t)allocator_sqlite3_round_(n), 0
    );
}

static inline int allocator_sqlite3_size_fn(void *ptr) {
    return (int)allocator_sized_size(ptr);
}

static inline int allocator_sqlite3_roundup_fn(int n) {
    return allocator_sqlite3_round_(n);
}

static inline int allocator_sqlite3_init_fn(void *app_data) {
    (void)app_data;
    return SQLITE_OK;
}

static inline void allocator_sqlite3_shutdown_fn(void *app_data) {
    (void)app_data;
}

/** `sqlite3_config(SQLITE_CONFIG_MALLOC, ...)` using `alloc`.
    Returns an SQLite result code. */
static inline int allocator_sqlite3_config(allocator_t *alloc) {
    sqlite3_mem_methods methods;
    allocator_t *previous = allocator_sqlite3_current_;
    int rc;

    if (!alloc)
        return SQLITE_NOMEM;
    memset(&methods, 0, sizeof methods);
    methods.xMalloc = &allocator_sqlite3_malloc_fn;
    methods.xFree = &allocator_sqlite3_free_fn;
    methods.xRealloc = &allocator_sqlite3_realloc_fn;
    methods.xSize = &allocator_sqlite3_size_fn;
    methods.xRoundup = &allocator_sqlite3_roundup_fn;
    methods.xInit = &allocator_sqlite3_init_fn;
    methods.xShutdown = &allocator_sqlite3_shutdown_fn;
    methods.pAppData = alloc;

    allocator_sqlite3_current_ = alloc;
    rc = sqlite3_config(SQLITE_CONFIG_MALLOC, &methods);
    if (rc != SQLITE_OK)
        allocator_sqlite3_current_ = previous;
    return rc;
}

#ifdef __cplusplus
}
#endif

#endif
