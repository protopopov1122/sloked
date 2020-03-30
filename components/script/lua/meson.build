lua_bindings_src = [
  'src/Common.cpp',
  'src/Editor.cpp',
  'src/Lua.cpp',
  'src/Logger.cpp',
  'src/Pipe.cpp',
  'src/Sched.cpp',
  'src/Server.cpp'
]

lua = dependency('lua', required : false)
threads = dependency('threads')
if not lua.found()
	lua = dependency('lua-5.3', required : true)
endif
sloked_lua_bindings_flags = ['-DSLOKED_FEATURE_SCRIPTING', '-DSLOKED_FEATURE_SCRIPTING_LUA']
sloked_lua_bindings_lib = static_library('sloked_lua', lua_bindings_src,
  cpp_args : sloked_lua_bindings_flags,
  include_directories : include_directories('headers'),
  dependencies : [lua, threads, sloked_framework])
sloked_lua_bindings = [declare_dependency(link_with : sloked_lua_bindings_lib,
  include_directories : include_directories('headers')), lua]
