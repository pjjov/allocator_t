/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for libgit2 (`GIT_OPT_SET_ALLOCATOR`).

    Library-wide hook: this library only offers process-global memory functions
    without a user-data pointer, so the allocator is kept in a file-local
    variable and shared by every call made through this header.
      - Install it once, from a single translation unit, before the library
        is used for anything else (and before any thread is started).
      - Never switch allocators while blocks from the previous one are alive.
      - The allocator must be thread-safe if the library is used from
        several threads, and must outlive the library.
      - A NULL allocator is reported as an out-of-memory error.

    Call `allocator_libgit2_set` before `git_libgit2_init` (memory allocated
    earlier would be freed by the new allocator). Returns 0 on success and a
    negative value on failure.

        allocator_libgit2_set(&my_arena);
        git_libgit2_init();

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

#ifndef ALLOCATOR_LIBGIT2
#define ALLOCATOR_LIBGIT2

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <git2.h>
#include <git2/sys/alloc.h>
#if defined(_MSC_VER)
    #define ALLOCATOR_LIBGIT2_CALL __cdecl
#else
    #define ALLOCATOR_LIBGIT2_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

static allocator_t *allocator_libgit2_current_ = NULL;

static inline void *ALLOCATOR_LIBGIT2_CALL
allocator_libgit2_malloc_fn(size_t size, const char *file, int line) {
    (void)file;
    (void)line;
    return allocator_sized_alloc(allocator_libgit2_current_, size, 0, 0);
}

static inline void *ALLOCATOR_LIBGIT2_CALL allocator_libgit2_realloc_fn(
    void *ptr, size_t size, const char *file, int line
) {
    (void)file;
    (void)line;
    return allocator_sized_realloc(allocator_libgit2_current_, ptr, size, 0);
}

static inline void ALLOCATOR_LIBGIT2_CALL allocator_libgit2_free_fn(void *ptr) {
    allocator_sized_free(allocator_libgit2_current_, ptr);
}

/** `git_libgit2_opts(GIT_OPT_SET_ALLOCATOR, ...)` using `alloc`. */
static inline int allocator_libgit2_set(allocator_t *alloc) {
    git_allocator ga;
    allocator_t *previous = allocator_libgit2_current_;
    int rc;

    if (!alloc)
        return -1;
    ga.gmalloc = &allocator_libgit2_malloc_fn;
    ga.grealloc = &allocator_libgit2_realloc_fn;
    ga.gfree = &allocator_libgit2_free_fn;
    allocator_libgit2_current_ = alloc;
    rc = git_libgit2_opts(GIT_OPT_SET_ALLOCATOR, &ga);
    if (rc < 0)
        allocator_libgit2_current_ = previous;
    return rc;
}

#ifdef __cplusplus
}
#endif

#endif
