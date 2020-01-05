lua_bindings_src = [
  'Common.cpp',
  'Lua.cpp',
  'Logger.cpp',
  'Pipe.cpp',
  'Sched.cpp',
  'Server.cpp'
]

lua = dependency('lua')
sloked_lua_bindings_lib = static_library('sloked_lua', lua_bindings_src,
  include_directories : include_directories('../../../headers/script/lua'),
  dependencies : [lua, sloked_framework])
sloked_lua_bindings = declare_dependency(link_with : sloked_lua_bindings_lib,
  include_directories : include_directories('../../../headers/script/lua'))