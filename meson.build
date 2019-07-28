project('sloked', 'cpp', 'c', default_options : ['cpp_std=c++17', 'warning_level=3'])

if meson.get_compiler('cpp').get_id() == 'gcc' or meson.get_compiler('cpp').get_id() == 'clang'
  add_project_arguments('-Wno-unused-parameter', '-pedantic', language : 'cpp')
endif

sloked_src = [
  'src/screen/terminal/posix/PosixTerminal.cpp',
  'src/screen/terminal/multiplexer/TerminalWindow.cpp',
  'src/screen/terminal/multiplexer/TerminalBuffer.cpp',
  'src/screen/terminal/multiplexer/TerminalSplitter.cpp',
  'src/screen/terminal/multiplexer/TerminalTabber.cpp',
  'src/screen/terminal/screen/ComponentHandle.cpp',
  'src/screen/terminal/screen/TextPaneComponent.cpp',
  'src/screen/terminal/screen/SplitterComponent.cpp',
  'src/screen/terminal/screen/TabberComponent.cpp',
  'src/screen/terminal/screen/TextPane.cpp',
  'src/main.cpp',
  'src/core/Error.cpp',
  'src/core/NewLine.cpp',
  'src/core/Encoding.cpp',
  'src/screen/terminal/CharWidth.cpp',
  'src/screen/Component.cpp',
  'src/screen/Splitter.cpp',
  'src/text/TextBlock.cpp',
  'src/text/TextChunk.cpp',
  'src/text/posix/TextFile.cpp',
  'src/text/TextBlockHandle.cpp',
  'src/text/TextRegion.cpp',
  'src/text/TextDocument.cpp',
  'src/text/TextView.cpp',
  'src/text/Cursor.cpp'
]

cpp = meson.get_compiler('cpp')
ncurses = dependency('ncurses')

sloked = executable('sloked', sloked_src,
  include_directories : include_directories('headers'),
  dependencies : [ncurses])