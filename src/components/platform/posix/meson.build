posix_platform_src = [
  'src/core/Library.cpp',
  'src/core/awaitable/Posix.cpp',
  'src/editor/PosixConfiguration.cpp',
  'src/filesystem/posix/File.cpp',
  'src/filesystem/posix/FileIO.cpp',
  'src/filesystem/posix/Reader.cpp',
  'src/filesystem/posix/View.cpp',
  'src/filesystem/posix/Writer.cpp',
  'src/namespace/posix/Environment.cpp',
  'src/namespace/posix/Filesystem.cpp',
  'src/net/PosixSocket.cpp',
  'src/screen/terminal/posix/PosixTerminal.cpp'
]

atomic = cpp.find_library('atomic', required : false)
ncurses = cpp.find_library('ncurses')
dl = cpp.find_library('dl')
sloked_posix_platform_flags = '-DSLOKED_PLATFORM_POSIX'
sloked_posix_platform_lib = static_library('sloked_posix_platform', posix_platform_src,
  include_directories : include_directories('headers'),
  cpp_args: sloked_posix_platform_flags,
  dependencies : [ncurses, dl, atomic, sloked_framework])
sloked_posix_platform = declare_dependency(link_with : sloked_posix_platform_lib,
  include_directories : include_directories('headers'))
 
sloked_platform = sloked_posix_platform
sloked_platform_flags = sloked_posix_platform_flags
