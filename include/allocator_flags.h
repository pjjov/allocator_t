/** `allocator_t` - Interface for custom allocators in C.

    This file provides functions for storing flags inside the allocator's
    pointer by using the type's minimum required aligment.

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

#ifndef ALLOCATOR_FLAGS_H
#define ALLOCATOR_FLAGS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ALLOCATOR_H
    #include "allocator.h"
#endif

#include <stddef.h>
#include <stdint.h>

#ifndef ALLOCATOR_ALIGNOF
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
        #define ALLOCATOR_ALIGNOF(type) _Alignof(type)
    #elif defined(__GNUC__) || defined(__clang__)
        #define ALLOCATOR_ALIGNOF(type) __alignof__(type)
    #elif defined(_MSC_VER)
        #define ALLOCATOR_ALIGNOF(type) __alignof(type)
    #else
        #define ALLOCATOR_ALIGNOF(type) \
            offsetof(                   \
                struct {                \
                    char c;             \
                    type member;        \
                },                      \
                member                  \
            )
    #endif
#endif

#define ALLOCATOR_FLAGS_MASK ((uintptr_t)(ALLOCATOR_ALIGNOF(allocator_t) - 1))

static inline allocator_t *allocator_flags_mask(allocator_t *allocator) {
    uintptr_t bits = (uintptr_t)allocator;
    return (allocator_t *)(bits & ~ALLOCATOR_FLAGS_MASK);
}

static inline allocator_t *allocator_flags_set(
    allocator_t *allocator, uintptr_t flags
) {
    uintptr_t bits = (uintptr_t)allocator;
    uintptr_t result = (bits & ~ALLOCATOR_FLAGS_MASK)
        | (flags & ALLOCATOR_FLAGS_MASK);

    return (allocator_t *)result;
}

static inline uintptr_t allocator_flags_get(const allocator_t *allocator) {
    return (uintptr_t)allocator & ALLOCATOR_FLAGS_MASK;
}

#ifdef __cplusplus
}
#endif

#endif