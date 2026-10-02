/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for libpng (requires `PNG_USER_MEM_SUPPORTED`, the default).

    Drop-in replacements for `png_create_read_struct` and
    `png_create_write_struct` that take an allocator instead of the error
    handler's memory arguments. Every later allocation of the png struct, its
    info structs and the internal zlib stream goes through the allocator, which
    must outlive them. Destroy with the regular `png_destroy_*_struct`.

        png_structp png = allocator_libpng_create_read_struct(
            PNG_LIBPNG_VER_STRING, NULL, NULL, NULL, &my_arena);

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

#ifndef ALLOCATOR_LIBPNG
#define ALLOCATOR_LIBPNG

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <png.h>
#ifndef PNG_USER_MEM_SUPPORTED
    #error                                                                     \
        "allocator_libpng.h requires libpng built with PNG_USER_MEM_SUPPORTED"
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline png_voidp allocator_libpng_malloc_fn(
    png_structp png, png_alloc_size_t size
) {
    return allocator_sized_alloc(
        (allocator_t *)png_get_mem_ptr(png), (size_t)size, 0, 0
    );
}

static inline void allocator_libpng_free_fn(png_structp png, png_voidp ptr) {
    allocator_sized_free((allocator_t *)png_get_mem_ptr(png), ptr);
}

/** `png_create_read_struct` using `alloc`. */
static inline png_structp allocator_libpng_create_read_struct(
    png_const_charp user_png_ver,
    png_voidp error_ptr,
    png_error_ptr error_fn,
    png_error_ptr warn_fn,
    allocator_t *alloc
) {
    return png_create_read_struct_2(
        user_png_ver,
        error_ptr,
        error_fn,
        warn_fn,
        alloc,
        &allocator_libpng_malloc_fn,
        &allocator_libpng_free_fn
    );
}

/** `png_create_write_struct` using `alloc`. */
static inline png_structp allocator_libpng_create_write_struct(
    png_const_charp user_png_ver,
    png_voidp error_ptr,
    png_error_ptr error_fn,
    png_error_ptr warn_fn,
    allocator_t *alloc
) {
    return png_create_write_struct_2(
        user_png_ver,
        error_ptr,
        error_fn,
        warn_fn,
        alloc,
        &allocator_libpng_malloc_fn,
        &allocator_libpng_free_fn
    );
}

#ifdef __cplusplus
}
#endif

#endif
