/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for libevent 2.x (`event_set_mem_functions`).

    Library-wide hook: this library only offers process-global memory functions
    without a user-data pointer, so the allocator is kept in a file-local
    variable and shared by every call made through this header.
      - Install it once, from a single translation unit, before the library
        is used for anything else (and before any thread is started).
      - Never switch allocators while blocks from the previous one are alive.
      - The allocator must be thread-safe if the library is used from
        several threads, and must outlive the library.
      - A NULL allocator is reported as an out-of-memory error.

    Must be called before any other libevent function. Returns 0 on success
    and -1 for a NULL allocator. Not available if libevent was built with
    `EVENT__DISABLE_MM_REPLACEMENT`. Call `libevent_global_shutdown` when done
    to release the library's own global state.

        allocator_libevent_set(&my_arena);
        struct event_base *base = event_base_new();

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

#ifndef ALLOCATOR_LIBEVENT
#define ALLOCATOR_LIBEVENT

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <event2/event.h>

#ifdef __cplusplus
extern "C" {
#endif

static allocator_t *allocator_libevent_current_ = NULL;

static inline void *allocator_libevent_malloc_fn(size_t size) {
    return allocator_sized_alloc(allocator_libevent_current_, size, 0, 0);
}

static inline void *allocator_libevent_realloc_fn(void *ptr, size_t size) {
    return allocator_sized_realloc(allocator_libevent_current_, ptr, size, 0);
}

static inline void allocator_libevent_free_fn(void *ptr) {
    allocator_sized_free(allocator_libevent_current_, ptr);
}

/** `event_set_mem_functions` using `alloc`. */
static inline int allocator_libevent_set(allocator_t *alloc) {
    if (!alloc)
        return -1;
    allocator_libevent_current_ = alloc;
    event_set_mem_functions(
        &allocator_libevent_malloc_fn,
        &allocator_libevent_realloc_fn,
        &allocator_libevent_free_fn
    );
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif
