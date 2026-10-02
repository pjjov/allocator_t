/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for zlib (and zlib-ng in compat mode).

    `z_stream` carries `zalloc`, `zfree` and `opaque`. Call
    `allocator_zlib_init` on the stream before `deflateInit`/`inflateInit`
    (or their `*2` and `*Init_` variants). The allocator must outlive the stream.

        z_stream strm = {0};
        allocator_zlib_init(&strm, &my_arena);
        deflateInit(&strm, Z_DEFAULT_COMPRESSION);

    The `gz*` file functions do not accept custom allocators.

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

#ifndef ALLOCATOR_ZLIB
#define ALLOCATOR_ZLIB

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <zlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline voidpf allocator_zlib_alloc_fn(
    voidpf opaque, uInt items, uInt size
) {
    size_t bytes = (size_t)items * size;
    if (size && bytes / size != items)
        return Z_NULL;
    return allocator_sized_alloc((allocator_t *)opaque, bytes, 0, 0);
}

static inline void allocator_zlib_free_fn(voidpf opaque, voidpf address) {
    allocator_sized_free((allocator_t *)opaque, address);
}

/** Makes the stream use `alloc`. Call before `deflateInit`/`inflateInit`. */
static inline void allocator_zlib_init(z_stream *strm, allocator_t *alloc) {
    strm->zalloc = &allocator_zlib_alloc_fn;
    strm->zfree = &allocator_zlib_free_fn;
    strm->opaque = (voidpf)alloc;
}

#ifdef __cplusplus
}
#endif

#endif
