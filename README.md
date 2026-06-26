<!--
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
-->

# allocator_t - Interface for custom allocators in C.

`allocator_t` is an interface for using and creating custom allocators
that enables you to swap in arenas, pools and debug allocators into
existing C libraries, thus helping you write safer and faster code.

With `allocator_t` you can break the global allocator paradigm
present in C and use the right tool for each situation.

You can use `allocator_t` with existing C libraries through
wrapper headers that are provided alongside this one.

## Behind the Scenes

`allocator_t` is built upon the following callback and structure:

```c
typedef void *(allocator_fn)(
    allocator_t *self,
    void *ptr,
    size_t old,
    size_t size,
    size_t zalign
);

typedef struct allocator_t {
    allocator_fn *const interface;
} allocator_t;
```

With only 5 parameters, this callback supports all allocator operations.
All you need to create a custom allocator is to implement an `allocator_fn` callback.

## Usage

While you can use the callback directly, it is recommended to use following
functions when interacting with allocators.

> NOTE: You should never store a dereferenced allocator, because it might have
> hidden members! Global allocators can be declared as `allocator_t`, but should
> be referenced when storing and calling: `&global_allocator`.

Implementing custom allocators is a matter of managing all the combinations
of parameters. The best way to learn this is looking at the implementation of
the following helper functions and using `allocator_std.h` as a reference.

### allocate

```c
void *allocate(allocator_t *alloc, size_t size);
```

Allocates a new memory block with `size` length. Equivalent to `malloc`.
Returns `NULL` if out of memory or `size == 0`.

### reallocate

```c
void *reallocate(allocator_t *alloc, void *ptr, size_t old, size_t size);
```

Resizes the allocation, previously allocated by the same allocator, either by expanding
or by moving the memory block pointed at by `ptr`. Depending on the allocator,
the `old` size might be required to resize the buffer. Based on `realloc`.

The implementation should try to preserve the alignment of the block if possible.

### deallocate

```c
void deallocate(allocator_t *alloc, void *ptr, size_t old);
```

Frees the memory block `ptr`. Depending on the allocator,
the `old` size might be required to free the memory.

### allocate_aligned

```c
void *allocate_aligned(allocator_t *alloc, size_t size, size_t align);
void *reallocate_aligned(allocator_t *alloc, void *ptr, size_t old, size_t size, size_t align);
```

These function variants ensure that the address of the returned memory block
is a multiple of `align`, which should be a power of 2.

Due to complexity, some allocators might not to implement alignment.
In that case, you can use `allocator_aligned.h` to extend their behaviour.

### zallocate

```c
void *zallocate(allocator_t *alloc, size_t size);
void *zallocate_aligned(allocator_t *alloc, size_t size, size_t align);
void *zreallocate(allocator_t *alloc, void *ptr, size_t old, size_t size);
void *zreallocate_aligned(allocator_t *alloc, void *ptr, size_t old, size_t size, size_t align);
```

Unlike their base variants, functions prefixed with **z** set the buffer's bits to 0,
like `calloc`. In case of realloc, only the bits after the old size are changed.

### x-prefixed variants

```c
void *xallocate(allocator_t *alloc, size_t size);
void *xallocate_aligned(allocator_t *alloc, size_t size, size_t align);
void *xreallocate(allocator_t *alloc, void *old, size_t size);
void *xreallocate_aligned(allocator_t *alloc, void *old, size_t size, size_t align);
void *xzallocate(allocator_t *alloc, size_t size);
void *xzallocate_aligned(allocator_t *alloc, size_t size, size_t align);
void *xzreallocate(allocator_t *alloc, void *ptr, size_t old, size_t size);
void *xzreallocate_aligned(allocator_t *alloc, void *ptr, size_t old, size_t size, size_t align);
```

Inspired by functions found in many projects already, functions prefixed with **x**
abort if out of memory (if `NULL` is returned by the base function). You can
define `allocator_failure` per inclusion to alter the behaviour of these functions.

### allocator_default

Like `allocator_failure`, `allocator_default` can be defined per inclusion to
alter the behaviour of functions in case a `NULL` allocator is passed.

### deallocate_all

```c
void deallocate_all(allocator_t *alloc);
```

This function asks the allocator to deallocate all of it's memory blocks.
Be aware, the behaviour is implementation defined and WILL NOT prevent
memory leaks. This function is intended for 'caching' allocators!

## Building

`allocator_t` is a header-only library, so just copy the header files you need
and include them in your build system. The interface is defined in `allocator.h`,
while wrappers for third-party libraries are in headers prefixed with `allocator_`.

| Header | Description |
|--------|-------------|
| [allocator.h](./include/allocator.h) | The `allocator_t` interface. |
| [allocator_aligned.h](./include/allocator_aligned.h) | Wraps allocators to provide aligned allocations. |
| [allocator_arena.h](./include/allocator_arena.h) | Simple arena/bump allocator implementation. |
| [allocator_print.h](./include/allocator_print.h) | Basic debug allocator that prints to a CSV file. |

Currently supported C libraries:

| Library | Header |
|---------|--------|
| C standard library | [allocator_std.h](./include/allocator_std.h) |

## License

See [LICENSE](./LICENSE) file for more information.
