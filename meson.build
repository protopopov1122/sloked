project('sloked', 'cpp', 'c', default_options : ['cpp_std=c++17', 'warning_level=3'])

if meson.get_compiler('cpp').get_id() == 'gcc' or meson.get_compiler('cpp').get_id() == 'clang'
  add_project_arguments('-Wno-unused-parameter', '-pedantic', language : 'cpp')
endif

sloked_src = [
  'src/PosixAnsiConsole.cpp',
  'src/main.cpp',
  'src/core/Error.cpp',
  'src/core/NewLine.cpp',
  'src/text/TextBlock.cpp',
  'src/text/TextChunk.cpp',
  'src/text/posix/TextFile.cpp',
  'src/text/TextBlockHandle.cpp',
  'src/text/TextRegion.cpp',
  'src/text/TextDocument.cpp'
]

sloked = executable('sloked', sloked_src,
  include_directories : include_directories('headers'))