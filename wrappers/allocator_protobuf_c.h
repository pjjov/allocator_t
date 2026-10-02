/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for protobuf-c.

    Returns a `ProtobufCAllocator` for `protobuf_c_message_unpack`,
    `protobuf_c_message_free_unpacked` and generated `*__unpack` /
    `*__free_unpacked` functions. protobuf-c does NOT copy the struct: keep it
    alive for as long as messages unpacked with it exist, and free them with
    the same allocator.

        ProtobufCAllocator pa = allocator_protobuf_c_allocator(&my_arena);
        Foo *foo = foo__unpack(&pa, len, data);
        foo__free_unpacked(foo, &pa);

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

#ifndef ALLOCATOR_PROTOBUF_C
#define ALLOCATOR_PROTOBUF_C

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#include <protobuf-c/protobuf-c.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *allocator_protobuf_c_alloc_fn(void *data, size_t size) {
    return allocator_sized_alloc((allocator_t *)data, size, 0, 0);
}

static inline void allocator_protobuf_c_free_fn(void *data, void *ptr) {
    allocator_sized_free((allocator_t *)data, ptr);
}

/** A `ProtobufCAllocator` using `alloc`. */
static inline ProtobufCAllocator allocator_protobuf_c_allocator(
    allocator_t *alloc
) {
    ProtobufCAllocator out;
    out.alloc = &allocator_protobuf_c_alloc_fn;
    out.free = &allocator_protobuf_c_free_fn;
    out.allocator_data = alloc;
    return out;
}

#ifdef __cplusplus
}
#endif

#endif
