win32_platform_src = [
  'src/core/Library.cpp',
  'src/core/awaitable/Win32.cpp',
  'src/editor/Win32Configuration.cpp',
  'src/filesystem/win32/File.cpp',
  'src/filesystem/win32/FileIO.cpp',
  'src/filesystem/win32/Reader.cpp',
  'src/filesystem/win32/Writer.cpp',
  'src/filesystem/win32/View.cpp',
  'src/namespace/win32/Environment.cpp',
  'src/namespace/win32/Filesystem.cpp',
  'src/net/Win32Socket.cpp'
]

atomic = cpp.find_library('atomic', required : false)
winsock2 = cpp.find_library('ws2_32')
shlwapi = cpp.find_library('shlwapi')
sloked_win32_platform_flags = '-DSLOKED_PLATFORM_WIN32'
sloked_win32_platform_lib = static_library('sloked_win32_platform', win32_platform_src,
  include_directories : include_directories('headers'),
  cpp_args: sloked_win32_platform_flags,
  dependencies : [sloked_framework, atomic, winsock2, shlwapi])
sloked_win32_platform = declare_dependency(link_with : sloked_win32_platform_lib,
  include_directories : include_directories('headers'))
 
sloked_platform = sloked_win32_platform
sloked_platform_flags = sloked_win32_platform_flags