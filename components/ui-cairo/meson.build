sloked_ui_cairo_sdl_src = [
  'src/screen/sdl/Event.cpp',
  'src/screen/sdl/Renderer.cpp',
  'src/screen/sdl/Surface.cpp',
  'src/screen/sdl/Texture.cpp',
  'src/screen/sdl/Window.cpp',
  # 'src/screen/terminal/CairoTerminal.cpp'
]

sdl2 = dependency('sdl2')
cairo = dependency('cairomm-1.0')
pango = dependency('pangomm-1.4')
sloked_ui_cairo_sdl_flags = '-DSLOKED_UI_CAIRO_SDL'
sloked_ui_cairo_sdl_lib = static_library('sloked_ui_sdl', sloked_ui_cairo_sdl_src,
  include_directories : include_directories('headers'),
  cpp_args: sloked_ui_cairo_sdl_flags,
  dependencies : [sloked_framework, sdl2, cairo, pango])
sloked_ui_cairo_sdl = declare_dependency(link_with : sloked_ui_cairo_sdl_lib,
  include_directories : include_directories('headers'),
  dependencies : [sdl2, cairo, pango])