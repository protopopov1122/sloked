
frontend_src = [
  'main.cpp'
]

frontend_flags = []
frontend_flags += sloked_compat_flags

frontend = executable('sloked', frontend_src,
  cpp_args : frontend_flags,
  dependencies : [sloked_framework, sloked_compat])