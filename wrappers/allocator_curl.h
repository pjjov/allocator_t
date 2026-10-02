/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for libcurl (`curl_global_init_mem`).

    Library-wide hook: this library only offers process-global memory functions
    without a user-data pointer, so the allocator is kept in a file-local
    variable and shared by every call made through this header.
      - Install it once, from a single translation unit, before the library
        is used for anything else (and before any thread is started).
      - Never switch allocators while blocks from the previous one are alive.
      - The allocator must be thread-safe if the library is used from
        several threads, and must outlive the library.
      - A NULL allocator is reported as an out-of-memory error.

    Use `allocator_curl_global_init` instead of `curl_global_init`, exactly once,
    before any other curl function, and finish with `curl_global_cleanup`.
    Returns a `CURLcode`. Memory returned by curl (`curl_easy_escape`,
    `curl_getenv`, ...) must be released with `curl_free`. libcurl's TLS
    backend may use its own allocator (e.g. OpenSSL, see `allocator_openssl.h`).

        allocator_curl_global_init(CURL_GLOBAL_DEFAULT, &my_arena);
        CURL *curl = curl_easy_init();

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

#ifndef ALLOCATOR_CURL
#define ALLOCATOR_CURL

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <curl/curl.h>

#ifdef __cplusplus
extern "C" {
#endif

static allocator_t *allocator_curl_current_ = NULL;

static inline void *allocator_curl_malloc_fn(size_t size) {
    return allocator_sized_alloc(allocator_curl_current_, size, 0, 0);
}

static inline void allocator_curl_free_fn(void *ptr) {
    allocator_sized_free(allocator_curl_current_, ptr);
}

static inline void *allocator_curl_realloc_fn(void *ptr, size_t size) {
    return allocator_sized_realloc(allocator_curl_current_, ptr, size, 0);
}

static inline char *allocator_curl_strdup_fn(const char *str) {
    return allocator_sized_strdup(allocator_curl_current_, str);
}

static inline void *allocator_curl_calloc_fn(size_t nmemb, size_t size) {
    return allocator_sized_calloc(allocator_curl_current_, nmemb, size);
}

/** `curl_global_init_mem` using `alloc`. */
static inline CURLcode allocator_curl_global_init(
    long flags, allocator_t *alloc
) {
    allocator_t *previous = allocator_curl_current_;
    CURLcode rc;

    if (!alloc)
        return CURLE_OUT_OF_MEMORY;
    allocator_curl_current_ = alloc;
    rc = curl_global_init_mem(
        flags,
        &allocator_curl_malloc_fn,
        &allocator_curl_free_fn,
        &allocator_curl_realloc_fn,
        &allocator_curl_strdup_fn,
        &allocator_curl_calloc_fn
    );
    if (rc != CURLE_OK)
        allocator_curl_current_ = previous;
    return rc;
}

#ifdef __cplusplus
}
#endif

#endif
