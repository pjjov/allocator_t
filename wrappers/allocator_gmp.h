/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for GMP (`mp_set_memory_functions`), also used by MPFR and others.

    Library-wide hook: this library only offers process-global memory functions
    without a user-data pointer, so the allocator is kept in a file-local
    variable and shared by every call made through this header.
      - Install it once, from a single translation unit, before the library
        is used for anything else (and before any thread is started).
      - Never switch allocators while blocks from the previous one are alive.
      - The allocator must be thread-safe if the library is used from
        several threads, and must outlive the library.
      - A NULL allocator is reported as an out-of-memory error.

    GMP passes exact block sizes to its reallocation and free functions, so
    this wrapper hands them straight to the allocator (no size headers): even
    allocators that need `old` work. GMP cannot handle allocation failure,
    so the aborting `xallocate` family is used (see `allocator_failure`).
    GMP may request zero bytes; those are served as one byte, consistently
    for allocation, reallocation and free.

    Returns 0 on success and -1 for a NULL allocator. Strings from
    `mpz_get_str(NULL, ...)` are released with
    `mp_get_memory_functions` free function, called with `strlen(s) + 1`.

        allocator_gmp_set(&my_arena);
        mpz_init(x);

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

#ifndef ALLOCATOR_GMP
#define ALLOCATOR_GMP

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#include <gmp.h>

#ifdef __cplusplus
extern "C" {
#endif

static allocator_t *allocator_gmp_current_ = NULL;

static inline size_t allocator_gmp_size_(size_t size) {
    return size ? size : 1;
}

static inline void *allocator_gmp_alloc_fn(size_t size) {
    return xallocate(allocator_gmp_current_, allocator_gmp_size_(size));
}

static inline void *allocator_gmp_realloc_fn(
    void *ptr, size_t old, size_t size
) {
    return xreallocate(
        allocator_gmp_current_,
        ptr,
        allocator_gmp_size_(old),
        allocator_gmp_size_(size)
    );
}

static inline void allocator_gmp_free_fn(void *ptr, size_t size) {
    deallocate(allocator_gmp_current_, ptr, allocator_gmp_size_(size));
}

/** `mp_set_memory_functions` using `alloc`. */
static inline int allocator_gmp_set(allocator_t *alloc) {
    if (!alloc)
        return -1;
    allocator_gmp_current_ = alloc;
    mp_set_memory_functions(
        &allocator_gmp_alloc_fn,
        &allocator_gmp_realloc_fn,
        &allocator_gmp_free_fn
    );
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif
