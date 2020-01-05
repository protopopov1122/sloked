lua_bindings_src = [
  'src/Common.cpp',
  'src/Lua.cpp',
  'src/Logger.cpp',
  'src/Pipe.cpp',
  'src/Sched.cpp',
  'src/Server.cpp'
]

lua = dependency('lua')
sloked_lua_bindings_lib = static_library('sloked_lua', lua_bindings_src,
  include_directories : include_directories('headers'),
  dependencies : [lua, sloked_framework])
sloked_lua_bindings = declare_dependency(link_with : sloked_lua_bindings_lib,
  include_directories : include_directories('headers'))