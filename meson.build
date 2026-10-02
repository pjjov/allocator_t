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
    'core/allocator_sized.h',
    'core/allocator_std.h',
    'core/allocator_tracker.h',
    'core/allocator.h',
    'wrappers/allocator_brotli.h',
    'wrappers/allocator_bzip2.h',
    'wrappers/allocator_cares.h',
    'wrappers/allocator_cjson.h',
    'wrappers/allocator_curl.h',
    'wrappers/allocator_duktape.h',
    'wrappers/allocator_freetype.h',
    'wrappers/allocator_glfw.h',
    'wrappers/allocator_gmp.h',
    'wrappers/allocator_hiredis.h',
    'wrappers/allocator_jansson.h',
    'wrappers/allocator_libevent.h',
    'wrappers/allocator_libgit2.h',
    'wrappers/allocator_liblzma.h',
    'wrappers/allocator_libpng.h',
    'wrappers/allocator_libuv.h',
    'wrappers/allocator_lua.h',
    'wrappers/allocator_miniaudio.h',
    'wrappers/allocator_nghttp2.h',
    'wrappers/allocator_openssl.h',
    'wrappers/allocator_pcre2.h',
    'wrappers/allocator_protobuf_c.h',
    'wrappers/allocator_sdl.h',
    'wrappers/allocator_sqlite3.h',
    'wrappers/allocator_vulkan.h',
    'wrappers/allocator_yajl.h',
    'wrappers/allocator_zlib.h',
    'wrappers/allocator_zstd.h',
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
