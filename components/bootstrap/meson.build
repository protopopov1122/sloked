
bootstrap_src = [
  'src/main.cpp',
  'src/Namespace.cpp',
  'src/Graphics.cpp'
]

bootstrap_flags = []
bootstrap_flags += sloked_compat_flags

bootstrap = executable('sloked', bootstrap_src,
  cpp_args : bootstrap_flags,
  include_directories : include_directories('headers'),
  dependencies : [sloked_framework, sloked_compat, sloked_frontend])