sloked_ui_sdl_src = [
  'src/screen/sdl/Event.cpp',
  'src/screen/sdl/Font.cpp',
  'src/screen/sdl/Surface.cpp',
  'src/screen/sdl/Texture.cpp',
  'src/screen/sdl/Window.cpp',
  'src/screen/terminal/SDL.cpp'
]

sdl2 = dependency('sdl2')
sdl_ttf = dependency('SDL2_ttf')
sloked_ui_sdl_flags = '-DSLOKED_UI_SDL'
sloked_ui_sdl_lib = static_library('sloked_ui_sdl', sloked_ui_sdl_src,
  include_directories : include_directories('headers'),
  cpp_args: sloked_ui_sdl_flags,
  dependencies : [sloked_framework, sdl2, sdl_ttf])
sloked_ui_sdl = declare_dependency(link_with : sloked_ui_sdl_lib,
  include_directories : include_directories('headers'))