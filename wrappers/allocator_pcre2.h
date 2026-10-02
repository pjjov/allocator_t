/** `allocator_t` - Interface for custom allocators in C.

    Wrapper for PCRE2.

    Define `PCRE2_CODE_UNIT_WIDTH` (8, 16 or 32) before including this file,
    as required by <pcre2.h>. The returned general context is what PCRE2 copies
    the memory functions from; pass it (or a compile context created from it)
    to `pcre2_compile`, `pcre2_match_data_create`, etc. Free it with
    `pcre2_general_context_free`. The allocator must outlive everything
    created with the context.

        pcre2_general_context *gc = allocator_pcre2_general_context(&my_arena);
        pcre2_compile_context *cc = pcre2_compile_context_create(gc);

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

#ifndef ALLOCATOR_PCRE2
#define ALLOCATOR_PCRE2

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#ifndef ALLOCATOR_SIZED
    #include "allocator_sized.h"
#endif

#ifndef PCRE2_CODE_UNIT_WIDTH
    #error "define PCRE2_CODE_UNIT_WIDTH before including allocator_pcre2.h"
#endif
#include <pcre2.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *allocator_pcre2_malloc_fn(PCRE2_SIZE size, void *data) {
    return allocator_sized_alloc((allocator_t *)data, (size_t)size, 0, 0);
}

static inline void allocator_pcre2_free_fn(void *ptr, void *data) {
    allocator_sized_free((allocator_t *)data, ptr);
}

/** A general context using `alloc`. Returns NULL on failure. */
static inline pcre2_general_context *allocator_pcre2_general_context(
    allocator_t *alloc
) {
    return pcre2_general_context_create(
        &allocator_pcre2_malloc_fn, &allocator_pcre2_free_fn, alloc
    );
}

#ifdef __cplusplus
}
#endif

#endif
