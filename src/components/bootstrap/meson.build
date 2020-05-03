
bootstrap_src = [
  'src/main.cpp',
  'src/Namespace.cpp',
  'src/Graphics.cpp'
]

bootstrap_flags = []
bootstrap_flags += sloked_compat_flags
bootstrap_dependencies = [sloked_framework, sloked_compat]

if get_option('default_application')
  bootstrap_flags += '-DSLOKED_BOOTSTRAP_HAS_DEFAULT_APPLICATION'
  bootstrap_dependencies += sloked_frontend
endif

bootstrap = executable('sloked', bootstrap_src,
  cpp_args : bootstrap_flags,
  include_directories : include_directories('headers'),
  dependencies : bootstrap_dependencies)