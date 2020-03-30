compat_src = [
  'src/Compat.cpp',
  'src/compression/Compat.cpp',
  'src/core/awaitable/Compat.cpp',
  'src/crypto/Compat.cpp',
  'src/editor/configuration/Compat.cpp',
  'src/namespace/Compat.cpp',
  'src/net/Compat.cpp',
  'src/screen/graphics/Compat.cpp',
  'src/screen/terminal/Compat.cpp',
  'src/screen/terminal/TerminalResize.cpp',
  'src/script/Compat.cpp'
]

sloked_compat_flags = []
sloked_compat_deps = [sloked_framework]
sloked_compat_flags += sloked_platform_flags
sloked_compat_deps += sloked_platform

if not sloked_disable_crypto
  sloked_compat_flags += sloked_crypto_flags
  sloked_compat_deps += sloked_crypto
endif

if not sloked_disable_compression
  sloked_compat_flags += sloked_compression_flags
  sloked_compat_deps += sloked_compression
endif

if not sloked_disable_scripting
  sloked_compat_flags += sloked_scripting_flags
  sloked_compat_deps += sloked_scripting
endif

if not sloked_disable_graphics
  sloked_compat_flags += sloked_graphics_flags
  sloked_compat_deps += sloked_graphics
endif

sloked_compat_lib = static_library('sloked_compat', compat_src,
  cpp_args : sloked_compat_flags,
  include_directories : include_directories('headers'),
  dependencies : sloked_compat_deps)
sloked_compat = declare_dependency(link_with : sloked_compat_lib,
  include_directories : include_directories('headers'))