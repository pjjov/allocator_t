/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for SDL2 (2.0.7+) and SDL3 (`SDL_SetMemoryFunctions`).

    Library-wide hook: this library only offers process-global memory functions
    without a user-data pointer, so the allocator is kept in a file-local
    variable and shared by every call made through this header.
      - Install it once, from a single translation unit, before the library
        is used for anything else (and before any thread is started).
      - Never switch allocators while blocks from the previous one are alive.
      - The allocator must be thread-safe if the library is used from
        several threads, and must outlive the library.
      - A NULL allocator is reported as an out-of-memory error.

    Call `allocator_sdl_set` before `SDL_Init`. It returns 0 on success and
    -1 on failure for both SDL versions. Define `ALLOCATOR_SDL3` to build
    against SDL3 instead of SDL2.

        allocator_sdl_set(&my_arena);
        SDL_Init(SDL_INIT_VIDEO);

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

#ifndef ALLOCATOR_SDL
#define ALLOCATOR_SDL

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#if defined(ALLOCATOR_SDL3)
    #include <SDL3/SDL_stdinc.h>
#elif defined(__has_include)
    #if __has_include(<SDL2/SDL_stdinc.h>)
        #include <SDL2/SDL_stdinc.h>
    #else
        #include <SDL_stdinc.h>
    #endif
#else
    #include <SDL2/SDL_stdinc.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static allocator_t *allocator_sdl_current_ = NULL;

static inline void *SDLCALL allocator_sdl_malloc_fn(size_t size) {
    return allocator_sized_alloc(allocator_sdl_current_, size, 0, 0);
}

static inline void *SDLCALL allocator_sdl_calloc_fn(size_t nmemb, size_t size) {
    return allocator_sized_calloc(allocator_sdl_current_, nmemb, size);
}

static inline void *SDLCALL allocator_sdl_realloc_fn(void *mem, size_t size) {
    return allocator_sized_realloc(allocator_sdl_current_, mem, size, 0);
}

static inline void SDLCALL allocator_sdl_free_fn(void *mem) {
    allocator_sized_free(allocator_sdl_current_, mem);
}

/** `SDL_SetMemoryFunctions` using `alloc`. Returns 0 on success, else -1. */
static inline int allocator_sdl_set(allocator_t *alloc) {
    allocator_t *previous = allocator_sdl_current_;
    int ok;

    if (!alloc)
        return -1;
    allocator_sdl_current_ = alloc;
#if defined(ALLOCATOR_SDL3)
    ok = SDL_SetMemoryFunctions(
             &allocator_sdl_malloc_fn,
             &allocator_sdl_calloc_fn,
             &allocator_sdl_realloc_fn,
             &allocator_sdl_free_fn
         )
        ? 0
        : -1;
#else
    ok = SDL_SetMemoryFunctions(
        &allocator_sdl_malloc_fn,
        &allocator_sdl_calloc_fn,
        &allocator_sdl_realloc_fn,
        &allocator_sdl_free_fn
    );
#endif
    if (ok != 0)
        allocator_sdl_current_ = previous;
    return ok;
}

#ifdef __cplusplus
}
#endif

#endif
