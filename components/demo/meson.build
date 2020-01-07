
demo_src = [
  'main.cpp'
]

demo_editor_flags = []
demo_editor_flags += sloked_compat_flags

demo_editor = executable('sloked', demo_src,
  cpp_args : demo_editor_flags,
  dependencies : [sloked_framework, sloked_compat])