/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for OpenSSL 1.1.0+ / 3.x (`CRYPTO_set_mem_functions`).

    Library-wide hook: this library only offers process-global memory functions
    without a user-data pointer, so the allocator is kept in a file-local
    variable and shared by every call made through this header.
      - Install it once, from a single translation unit, before the library
        is used for anything else (and before any thread is started).
      - Never switch allocators while blocks from the previous one are alive.
      - The allocator must be thread-safe if the library is used from
        several threads, and must outlive the library.
      - A NULL allocator is reported as an out-of-memory error.

    Returns 1 on success and 0 on failure, like OpenSSL. OpenSSL rejects the
    call once it has allocated with its default allocator, so make it the first
    OpenSSL call of the process (before `OPENSSL_init_ssl`, ...). OpenSSL does
    NOT reject a later swap of a custom hook, so this wrapper does: calling it
    again with the same allocator succeeds, with a different one it fails.
    Call `OPENSSL_cleanup` before destroying the allocator. The secure heap
    (`OPENSSL_secure_malloc`) is a separate facility and is not affected.
    Other libraries that use OpenSSL internally (curl, libssh, ...) share
    the hook.

        allocator_openssl_set(&my_arena);
        OPENSSL_init_ssl(0, NULL);

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

#ifndef ALLOCATOR_OPENSSL
#define ALLOCATOR_OPENSSL

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <openssl/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

static allocator_t *allocator_openssl_current_ = NULL;

static inline void *allocator_openssl_malloc_fn(
    size_t size, const char *file, int line
) {
    (void)file;
    (void)line;
    return allocator_sized_alloc(allocator_openssl_current_, size, 0, 0);
}

static inline void *allocator_openssl_realloc_fn(
    void *ptr, size_t size, const char *file, int line
) {
    (void)file;
    (void)line;
    return allocator_sized_realloc(allocator_openssl_current_, ptr, size, 0);
}

static inline void allocator_openssl_free_fn(
    void *ptr, const char *file, int line
) {
    (void)file;
    (void)line;
    allocator_sized_free(allocator_openssl_current_, ptr);
}

/** `CRYPTO_set_mem_functions` using `alloc`. Returns 1 on success. */
static inline int allocator_openssl_set(allocator_t *alloc) {
    allocator_t *previous = allocator_openssl_current_;
    int ok;

    if (!alloc)
        return 0;
    if (previous)
        return previous == alloc; /* never swap while blocks may be alive */
    allocator_openssl_current_ = alloc;
    ok = CRYPTO_set_mem_functions(
        &allocator_openssl_malloc_fn,
        &allocator_openssl_realloc_fn,
        &allocator_openssl_free_fn
    );
    if (!ok)
        allocator_openssl_current_ = previous;
    return ok;
}

#ifdef __cplusplus
}
#endif

#endif
