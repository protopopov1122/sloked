
frontend_src = [
  'src/CorePlugin.cpp'
]

sloked_frontend_lib = shared_library('sloked_frontend_core', frontend_src,
  dependencies : [sloked_framework])
sloked_frontend = [declare_dependency(link_with : sloked_frontend_lib)]