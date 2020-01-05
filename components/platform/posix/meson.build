posix_platform_src = [
  'src/core/awaitable/Posix.cpp',
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

ncurses = dependency('ncurses')
sloked_posix_platform_lib = static_library('sloked_posix_platform', posix_platform_src,
  include_directories : include_directories('headers'),
  dependencies : [ncurses, sloked_framework])
sloked_posix_platform = declare_dependency(link_with : sloked_posix_platform_lib,
  include_directories : include_directories('headers'))