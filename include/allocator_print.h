/** `allocator_t` - Interface for custom allocators in C.

    This files provides a wrapper allocator that can be used for basic
    logging and debugging. It writes the arguments for each allocator
    call to a CSV log file, or standard output.

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

#ifndef ALLOCATOR_PRINT_H
#define ALLOCATOR_PRINT_H

#ifndef ALLOCATOR_T
    #include "allocator.h"
#endif

#include <stdio.h>
#include <time.h>

struct print_alloc {
    allocator_t alloc;
    allocator_t *base;
    FILE *log;
};

static void *print_alloc_fn(
    allocator_t *self, void *ptr, size_t old, size_t size, size_t zalign
) {
    struct print_alloc *alloc = (struct print_alloc *)self;
    void *out = allocator_call(alloc->base, ptr, old, size, zalign);

    if (alloc->log) {
        fprintf(
            alloc->log,
            "%lu,%lf,%p,%p,%p,%lu,%lu,%lu,%s,%p\n",
            (unsigned long)time(NULL),
            (double)clock() / CLOCKS_PER_SEC,
            self,
            alloc->base,
            ptr,
            (unsigned long)old,
            (unsigned long)size,
            (unsigned long)(zalign & ~1),
            zalign & 1 ? "true" : "false",
            out
        );
    }
    return out;
}

static inline void print_alloc_init(
    struct print_alloc *out, FILE *log, allocator_t *base
) {
    allocator_fn **interface = (allocator_fn **)&out->alloc.interface;
    *interface = &print_alloc_fn;
    out->base = base;
    out->log = log ? log : stdout;
    if (log) {
        fputs(
            "System time,"
            "Clock time,"
            "Log Allocator,"
            "Base Allocator,"
            "Old pointer,"
            "Old size,"
            "Requested size,"
            "Requested alignment,"
            "Clear the buffer,"
            "Resulting buffer\n",
            out->log
        );
    }
}

#endif
