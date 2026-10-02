/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for GLFW 3.4 or newer.

    Returns a `GLFWallocator` for `glfwInitAllocator`, which has to be called
    before `glfwInit` from the main thread. GLFW copies the struct; the
    allocator must outlive `glfwTerminate`. GLFW never passes a size of 0 or
    a NULL block to the reallocation callback, and it may call the callbacks
    from any thread, so the allocator must be thread-safe.

        GLFWallocator ga = allocator_glfw_allocator(&my_arena);
        glfwInitAllocator(&ga);
        glfwInit();

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

#ifndef ALLOCATOR_GLFW
#define ALLOCATOR_GLFW

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <GLFW/glfw3.h>
#if GLFW_VERSION_MAJOR < 3                                 \
    || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR < 4)
    #error "allocator_glfw.h requires GLFW 3.4 or newer"
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline void *allocator_glfw_allocate_fn(size_t size, void *user) {
    return allocator_sized_alloc((allocator_t *)user, size, 0, 0);
}

static inline void *allocator_glfw_reallocate_fn(
    void *block, size_t size, void *user
) {
    return allocator_sized_realloc((allocator_t *)user, block, size, 0);
}

static inline void allocator_glfw_deallocate_fn(void *block, void *user) {
    allocator_sized_free((allocator_t *)user, block);
}

/** A `GLFWallocator` using `alloc`. */
static inline GLFWallocator allocator_glfw_allocator(allocator_t *alloc) {
    GLFWallocator out;
    out.allocate = &allocator_glfw_allocate_fn;
    out.reallocate = &allocator_glfw_reallocate_fn;
    out.deallocate = &allocator_glfw_deallocate_fn;
    out.user = alloc;
    return out;
}

#ifdef __cplusplus
}
#endif

#endif
