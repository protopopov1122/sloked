win32_platform_src = [
  'src/Dummy.cpp'
]

sloked_win32_platform_flags = '-DSLOKED_PLATFORM_WIN32'
sloked_win32_platform_lib = static_library('sloked_win32_platform', win32_platform_src,
  include_directories : include_directories('headers'),
  cpp_args: sloked_win32_platform_flags,
  dependencies : [sloked_framework])
sloked_win32_platform = declare_dependency(link_with : sloked_win32_platform_lib,
  include_directories : include_directories('headers'))
 
sloked_platform = sloked_win32_platform
sloked_platform_flags = sloked_win32_platform_flags