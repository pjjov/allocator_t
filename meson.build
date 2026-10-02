# SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
# SPDX-License-Identifier: Apache-2.0

project('allocator_t', 'c', version: '1.1')

allocator_t_dep = declare_dependency(
    include_directories: include_directories('core', 'wrappers')
)

meson.override_dependency('allocator_t', allocator_t_dep)

install_headers(
    'core/allocator_aligned.h',
    'core/allocator_arena.h',
    'core/allocator_flags.h',
    'core/allocator_joined.h',
    'core/allocator_pool.h',
    'core/allocator_print.h',
    'core/allocator_std.h',
    'core/allocator_tracker.h',
    'core/allocator.h'
)

pkg = import('pkgconfig')
pkg.generate(
  name: 'allocator_t',
  filebase: 'allocator_t',
  version: meson.project_version(),
  description: 'Interface for custom allocators in C.',
  subdirs: '',
  extra_cflags: []
)
