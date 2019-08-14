project('sloked', 'cpp', 'c', default_options : ['cpp_std=c++17', 'warning_level=3'])

if meson.get_compiler('cpp').get_id() == 'gcc' or meson.get_compiler('cpp').get_id() == 'clang'
  add_project_arguments('-Wno-unused-parameter', '-pedantic', language : 'cpp')
endif

sloked_src = [
  'src/main.cpp',
  'src/core/CharWidth.cpp',
  'src/core/Encoding.cpp',
  'src/core/Error.cpp',
  'src/core/NewLine.cpp',
  'src/core/Position.cpp',
  'src/filesystem/FileIO.cpp',
  'src/filesystem/posix/File.cpp',
  'src/filesystem/posix/FileIO.cpp',
  'src/filesystem/posix/Reader.cpp',
  'src/filesystem/posix/View.cpp',
  'src/filesystem/posix/Writer.cpp',
  'src/namespace/Directory.cpp',
  'src/namespace/Filesystem.cpp',
  'src/namespace/Object.cpp',
  'src/namespace/Path.cpp',
  'src/namespace/View.cpp',
  'src/namespace/Virtual.cpp',
  'src/namespace/posix/Filesystem.cpp',
  'src/screen/Component.cpp',
  'src/screen/Splitter.cpp',
  'src/screen/terminal/multiplexer/TerminalWindow.cpp',
  'src/screen/terminal/multiplexer/TerminalBuffer.cpp',
  'src/screen/terminal/multiplexer/TerminalSplitter.cpp',
  'src/screen/terminal/multiplexer/TerminalTabber.cpp',
  'src/screen/terminal/posix/PosixTerminal.cpp',
  'src/screen/terminal/screen/ComponentHandle.cpp',
  'src/screen/terminal/screen/MultiplexerComponent.cpp',
  'src/screen/terminal/screen/SplitterComponent.cpp',
  'src/screen/terminal/screen/TabberComponent.cpp',
  'src/screen/terminal/screen/TextPane.cpp',
  'src/screen/terminal/screen/TextPaneComponent.cpp',
  'src/screen/widgets/TextEditor.cpp',
  'src/text/TextBlock.cpp',
  'src/text/TextBlockHandle.cpp',
  'src/text/TextChunk.cpp',
  'src/text/TextDocument.cpp',
  'src/text/TextFrame.cpp',
  'src/text/TextRegion.cpp',
  'src/text/TextView.cpp',
  'src/text/cursor/Cursor.cpp',
  'src/text/cursor/EditingPrimitives.cpp',
  'src/text/cursor/Transaction.cpp',
  'src/text/cursor/TransactionBatch.cpp',
  'src/text/cursor/TransactionCursor.cpp',
  'src/text/cursor/TransactionFactory.cpp',
  'src/text/cursor/TransactionStreamMultiplexer.cpp'
]

cpp = meson.get_compiler('cpp')
ncurses = dependency('ncurses')

sloked = executable('sloked', sloked_src,
  include_directories : include_directories('headers'),
  dependencies : [ncurses])