
frontend_src = [
  'main.cpp',
  'src/CorePlugin.cpp',
  'src/Graphics.cpp',
  'src/Namespace.cpp'
]

frontend_flags = []
frontend_flags += sloked_compat_flags

frontend = executable('sloked', frontend_src,
  cpp_args : frontend_flags,
  include_directories : include_directories('headers'),
  dependencies : [sloked_framework, sloked_compat])