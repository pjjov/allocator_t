/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for miniaudio.

    Returns a `ma_allocation_callbacks` for the `pAllocationCallbacks` parameter
    found in nearly every miniaudio `*_init` function. miniaudio copies the
    struct into the objects, so it can be a temporary; the allocator must
    outlive them, and must be thread-safe (the audio thread allocates).

    Include <miniaudio.h> yourself (with `MINIAUDIO_IMPLEMENTATION` in one
    translation unit) before this file, or let this file include it.

        ma_allocation_callbacks cb = allocator_miniaudio_callbacks(&my_arena);
        ma_engine_config cfg = ma_engine_config_init();
        cfg.allocationCallbacks = cb;

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

#ifndef ALLOCATOR_MINIAUDIO
#define ALLOCATOR_MINIAUDIO

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include "miniaudio.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void *allocator_miniaudio_malloc_fn(size_t size, void *user) {
    return allocator_sized_alloc((allocator_t *)user, size, 0, 0);
}

static inline void *allocator_miniaudio_realloc_fn(
    void *ptr, size_t size, void *user
) {
    return allocator_sized_realloc((allocator_t *)user, ptr, size, 0);
}

static inline void allocator_miniaudio_free_fn(void *ptr, void *user) {
    allocator_sized_free((allocator_t *)user, ptr);
}

/** Allocation callbacks using `alloc`. */
static inline ma_allocation_callbacks allocator_miniaudio_callbacks(
    allocator_t *alloc
) {
    ma_allocation_callbacks cb;
    cb.pUserData = alloc;
    cb.onMalloc = &allocator_miniaudio_malloc_fn;
    cb.onRealloc = &allocator_miniaudio_realloc_fn;
    cb.onFree = &allocator_miniaudio_free_fn;
    return cb;
}

#ifdef __cplusplus
}
#endif

#endif
