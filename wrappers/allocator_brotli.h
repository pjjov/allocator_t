/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for Brotli.

    Creates encoder/decoder instances that use the given allocator.
    Define `ALLOCATOR_BROTLI_NO_ENCODER` or `ALLOCATOR_BROTLI_NO_DECODER`
    before including this file if only one half of the library is installed.
    Destroy the instances with the regular `Brotli*DestroyInstance`.

        BrotliEncoderState *enc = allocator_brotli_encoder_create(&my_arena);

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

#ifndef ALLOCATOR_BROTLI
#define ALLOCATOR_BROTLI

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <brotli/types.h>
#ifndef ALLOCATOR_BROTLI_NO_ENCODER
    #include <brotli/encode.h>
#endif
#ifndef ALLOCATOR_BROTLI_NO_DECODER
    #include <brotli/decode.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline void *allocator_brotli_alloc_fn(void *opaque, size_t size) {
    return allocator_sized_alloc((allocator_t *)opaque, size, 0, 0);
}

static inline void allocator_brotli_free_fn(void *opaque, void *address) {
    allocator_sized_free((allocator_t *)opaque, address);
}

#ifndef ALLOCATOR_BROTLI_NO_ENCODER
/** `BrotliEncoderCreateInstance` using `alloc`. Returns NULL on failure. */
static inline BrotliEncoderState *allocator_brotli_encoder_create(
    allocator_t *alloc
) {
    return BrotliEncoderCreateInstance(
        &allocator_brotli_alloc_fn, &allocator_brotli_free_fn, alloc
    );
}
#endif

#ifndef ALLOCATOR_BROTLI_NO_DECODER
/** `BrotliDecoderCreateInstance` using `alloc`. Returns NULL on failure. */
static inline BrotliDecoderState *allocator_brotli_decoder_create(
    allocator_t *alloc
) {
    return BrotliDecoderCreateInstance(
        &allocator_brotli_alloc_fn, &allocator_brotli_free_fn, alloc
    );
}
#endif

#ifdef __cplusplus
}
#endif

#endif