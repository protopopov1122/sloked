compat_src = [
  'src/core/awaitable/Compat.cpp',
  'src/crypto/Compat.cpp',
  'src/namespace/Compat.cpp',
  'src/net/Compat.cpp',
  'src/screen/terminal/Compat.cpp',
  'src/screen/terminal/TerminalResize.cpp',
  'src/script/Compat.cpp'
]

sloked_compat_flags = []
sloked_compat_flags += sloked_platform_flags
sloked_compat_flags += sloked_scripting_flags
sloked_compat_flags += sloked_crypto_flags

sloked_compat_lib = static_library('sloked_compat', compat_src,
  cpp_args : sloked_compat_flags,
  include_directories : include_directories('headers'),
  dependencies : [sloked_framework, sloked_crypto, sloked_scripting, sloked_platform])
sloked_compat = declare_dependency(link_with : sloked_compat_lib,
  include_directories : include_directories('headers'))