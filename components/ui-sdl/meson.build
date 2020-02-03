sloked_ui_sdl_src = [
    'src/screen/ui/SDL.cpp'
]

sdl2 = dependency('sdl2')
sloked_ui_sdl_flags = '-DSLOKED_UI_SDL'
sloked_ui_sdl_lib = static_library('sloked_ui_sdl', sloked_ui_sdl_src,
  include_directories : include_directories('headers'),
  cpp_args: sloked_ui_sdl_flags,
  dependencies : [sloked_framework, sdl2])
sloked_ui_sdl = declare_dependency(link_with : sloked_ui_sdl_lib,
  include_directories : include_directories('headers'))