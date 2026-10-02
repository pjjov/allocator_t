/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for FreeType.

    `FT_Init_FreeType` always uses malloc, so a library with a custom allocator
    has to be assembled by hand: `FT_New_Library`, `FT_Add_Default_Modules` and
    `FT_Set_Default_Properties`. `allocator_freetype_new_library` does exactly
    that. The `struct FT_MemoryRec_` you pass in is stored by FreeType and must stay
    alive (and not move) for the lifetime of the library, as must the allocator.

    Destroy the library with `FT_Done_Library` (or the helper below), NEVER with
    `FT_Done_FreeType`, which would try to free the `struct FT_MemoryRec_` itself.

        static struct FT_MemoryRec_ memory;
        FT_Library library;
        allocator_freetype_new_library(&memory, &my_arena, &library);
        ...
        allocator_freetype_done_library(library);

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

#ifndef ALLOCATOR_FREETYPE
#define ALLOCATOR_FREETYPE

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H
#include FT_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

static inline void *allocator_freetype_alloc_fn(FT_Memory memory, long size) {
    if (size < 0)
        return NULL;
    return allocator_sized_alloc(
        (allocator_t *)memory->user, (size_t)size, 0, 0
    );
}

static inline void allocator_freetype_free_fn(FT_Memory memory, void *block) {
    allocator_sized_free((allocator_t *)memory->user, block);
}

static inline void *allocator_freetype_realloc_fn(
    FT_Memory memory, long cur_size, long new_size, void *block
) {
    (void)cur_size; /* the size header is authoritative */
    if (new_size < 0)
        return NULL;
    return allocator_sized_realloc(
        (allocator_t *)memory->user, block, (size_t)new_size, 0
    );
}

/** Fills `memory` so that FreeType uses `alloc`. */
static inline void allocator_freetype_memory_init(
    FT_Memory memory, allocator_t *alloc
) {
    memory->user = alloc;
    memory->alloc = &allocator_freetype_alloc_fn;
    memory->free = &allocator_freetype_free_fn;
    memory->realloc = &allocator_freetype_realloc_fn;
}

/** Like `FT_Init_FreeType`, but using `alloc`. `memory` must outlive it. */
static inline FT_Error allocator_freetype_new_library(
    FT_Memory memory, allocator_t *alloc, FT_Library *library
) {
    FT_Error error;
    allocator_freetype_memory_init(memory, alloc);
    error = FT_New_Library(memory, library);
    if (error)
        return error;
    FT_Add_Default_Modules(*library);
    FT_Set_Default_Properties(*library);
    return 0;
}

/** Counterpart of `allocator_freetype_new_library`. */
static inline FT_Error allocator_freetype_done_library(FT_Library library) {
    return FT_Done_Library(library);
}

#ifdef __cplusplus
}
#endif

#endif
