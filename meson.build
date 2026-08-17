# SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
# SPDX-License-Identifier: Apache-2.0

project('allocator_t', 'c', version: '1.1')

allocator_t_dep = declare_dependency(
    include_directories: include_directories('include')
)

meson.override_dependency('allocator_t', allocator_t_dep)

install_headers(
    'include/allocator_aligned.h',
    'include/allocator_arena.h',
    'include/allocator_flags.h',
    'include/allocator_joined.h',
    'include/allocator_pool.h',
    'include/allocator_print.h',
    'include/allocator_std.h',
    'include/allocator.h'
)

pkg = import('pkgconfig')
pkg.generate(
  name: 'allocator_t',
  filebase: 'allocator_t',
  version: '1.1',
  description: 'Interface for custom allocators in C.',
  subdirs: '',
  extra_cflags: []
)