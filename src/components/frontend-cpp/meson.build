
frontend_cpp_src = [
  'src/Application.cpp'
]

frontend_cpp_args = []
frontend_cpp_dependencies = [sloked_framework]
if get_option('scripting_lua')
  frontend_cpp_args += sloked_lua_bindings_flags
  frontend_cpp_dependencies += sloked_lua_bindings
endif

sloked_frontend_cpp_lib = shared_library('sloked_frontend_cpp', frontend_cpp_src,
  cpp_args : frontend_cpp_args,
  dependencies : frontend_cpp_dependencies)