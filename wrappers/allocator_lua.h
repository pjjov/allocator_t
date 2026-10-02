/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for Lua 5.1 - 5.4 (and LuaJIT builds with `LJ_GC64`).

    `allocator_lua_fn` is a `lua_Alloc` that maps directly onto `allocator_t`
    using the exact block sizes Lua reports, so no size headers are needed and
    allocators that require `old` work as intended. When `ptr` is NULL Lua's
    `osize` encodes an object type instead of a size; this is handled.

    Lua assumes that shrinking a block never fails. Allocators used with Lua
    must therefore be able to shrink (or ignore shrinking of) their blocks.
    Lua also expects `malloc`-like alignment from plain allocations.

        lua_State *L = allocator_lua_newstate(&my_arena);
        luaL_openlibs(L);

    `lua_newstate` does not install a panic handler, unlike `luaL_newstate`;
    call `lua_atpanic` if you want one.

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

#ifndef ALLOCATOR_LUA
#define ALLOCATOR_LUA

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#include <lua.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *allocator_lua_fn(
    void *ud, void *ptr, size_t osize, size_t nsize
) {
    allocator_t *alloc = (allocator_t *)ud;
    if (nsize == 0) {
        if (ptr)
            deallocate(alloc, ptr, osize);
        return NULL;
    }
    if (!ptr) /* `osize` is a type tag here, not a size */
        return allocate(alloc, nsize);
    return reallocate(alloc, ptr, osize, nsize);
}

/** `lua_newstate` using `alloc`. Returns NULL on failure. */
static inline lua_State *allocator_lua_newstate(allocator_t *alloc) {
    return lua_newstate(&allocator_lua_fn, alloc);
}

#ifdef __cplusplus
}
#endif

#endif
