/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for Vulkan host allocation callbacks.

    Returns a `VkAllocationCallbacks` to pass as `pAllocator` to the `vkCreate*`,
    `vkDestroy*`, `vkAllocate*` and `vkFree*` functions. Vulkan requires the same
    callbacks (or equivalent) for creating and destroying an object, and may call
    them from several threads, so the allocator has to be thread-safe.

    Vulkan asks for alignments above 16 bytes (up to 64 or more), which are
    forwarded to `allocate_aligned`: the allocator must support alignment
    (wrap it with `allocator_aligned.h` otherwise). The alignment given to the
    reallocation callback is ignored, as Vulkan requires it to match the
    original allocation. The internal allocation notifications are not set.

        VkAllocationCallbacks vk = allocator_vulkan_callbacks(&my_arena);
        vkCreateInstance(&info, &vk, &instance);

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

#ifndef ALLOCATOR_VULKAN
#define ALLOCATOR_VULKAN

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <vulkan/vulkan_core.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline VKAPI_ATTR void *VKAPI_CALL allocator_vulkan_allocation_fn(
    void *user, size_t size, size_t alignment, VkSystemAllocationScope scope
) {
    (void)scope;
    return allocator_sized_alloc((allocator_t *)user, size, alignment, 0);
}

static inline VKAPI_ATTR void *VKAPI_CALL allocator_vulkan_reallocation_fn(
    void *user,
    void *original,
    size_t size,
    size_t alignment,
    VkSystemAllocationScope scope
) {
    (void)scope;
    if (!original)
        return allocator_sized_alloc((allocator_t *)user, size, alignment, 0);
    return allocator_sized_realloc((allocator_t *)user, original, size, 0);
}

static inline VKAPI_ATTR void VKAPI_CALL
allocator_vulkan_free_fn(void *user, void *memory) {
    allocator_sized_free((allocator_t *)user, memory);
}

/** Allocation callbacks using `alloc`. */
static inline VkAllocationCallbacks allocator_vulkan_callbacks(
    allocator_t *alloc
) {
    VkAllocationCallbacks cb;
    memset(&cb, 0, sizeof cb);
    cb.pUserData = alloc;
    cb.pfnAllocation = &allocator_vulkan_allocation_fn;
    cb.pfnReallocation = &allocator_vulkan_reallocation_fn;
    cb.pfnFree = &allocator_vulkan_free_fn;
    return cb;
}

#ifdef __cplusplus
}
#endif

#endif
