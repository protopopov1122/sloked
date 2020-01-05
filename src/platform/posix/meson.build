posix_platform_src = [
  'core/awaitable/Posix.cpp',
  'filesystem/posix/File.cpp',
  'filesystem/posix/FileIO.cpp',
  'filesystem/posix/Reader.cpp',
  'filesystem/posix/View.cpp',
  'filesystem/posix/Writer.cpp',
  'namespace/posix/Environment.cpp',
  'namespace/posix/Filesystem.cpp',
  'net/PosixSocket.cpp',
  'screen/terminal/posix/PosixTerminal.cpp'
]

ncurses = dependency('ncurses')
sloked_posix_platform_lib = static_library('sloked_posix_platform', posix_platform_src,
  include_directories : include_directories('../../../headers/platform/posix'),
  dependencies : [ncurses, sloked_framework])
sloked_posix_platform = declare_dependency(link_with : sloked_posix_platform_lib,
  include_directories : include_directories('../../../headers/platform/posix'))