
headless_src = [
  'src/Application.cpp'
]

sloked_headless_app_lib = shared_library('sloked_headless_app', headless_src,
  dependencies : [sloked_framework])
sloked_frontend = [declare_dependency(link_with : sloked_headless_app_lib)]