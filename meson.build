project('sloked', 'cpp', 'c', default_options : ['cpp_std=c++17', 'warning_level=3'])

compiler_id = meson.get_compiler('cpp').get_id()
if compiler_id == 'gcc' or compiler_id == 'clang'
  add_project_arguments('-Wno-unused-parameter', '-pedantic', language : 'cpp')
  add_project_link_arguments('-rdynamic', language : 'cpp')
elif compiler_id == 'pgi'
  add_project_arguments('-std=c++17', '-noswitcherror', language : 'cpp')
  add_project_link_arguments('-noswitcherror', '-rdynamic', language : 'cpp')
endif

sloked_src = [
  'src/framework/core/Base64.cpp',
  'src/framework/core/CharWidth.cpp',
  'src/framework/core/CLI.cpp',
  'src/framework/core/Closeable.cpp',
  'src/framework/core/DynamicBitset.cpp',
  'src/framework/core/Encoding.cpp',
  'src/framework/core/Error.cpp',
  'src/framework/core/Failure.cpp',
  'src/framework/core/Locale.cpp',
  'src/framework/core/Logger.cpp',
  'src/framework/core/NewLine.cpp',
  'src/framework/core/Position.cpp',
  'src/framework/core/Scope.cpp',
  'src/framework/core/Semaphore.cpp',
  'src/framework/core/String.cpp',
  'src/framework/core/ThreadPool.cpp',
  'src/framework/core/URI.cpp',
  'src/framework/core/encoding/Utf8.cpp',
  'src/framework/core/encoding/Utf32LE.cpp',
  'src/framework/core/awaitable/Poll.cpp',
  'src/framework/editor/Configuration.cpp',
  'src/framework/editor/EditorCore.cpp',
  'src/framework/editor/EditorServer.cpp',
  'src/framework/editor/ScreenServer.cpp',
  'src/framework/editor/doc-mgr/Document.cpp',
  'src/framework/editor/doc-mgr/DocumentSet.cpp',
  'src/framework/editor/doc-mgr/DocumentUpstream.cpp',
  'src/framework/editor/terminal/ScreenProvider.cpp',
  'src/framework/filesystem/FileIO.cpp',
  'src/framework/json/AST.cpp',
  'src/framework/json/Lexer.cpp',
  'src/framework/json/Parser.cpp',
  'src/framework/kgr/Path.cpp',
  'src/framework/kgr/RestrictedServer.cpp',
  'src/framework/kgr/Serialize.cpp',
  'src/framework/kgr/Value.cpp',
  'src/framework/kgr/local/Context.cpp',
  'src/framework/kgr/local/NamedServer.cpp',
  'src/framework/kgr/local/Pipe.cpp',
  'src/framework/kgr/local/Server.cpp',
  'src/framework/kgr/net/Interface.cpp',
  'src/framework/kgr/net/MasterServer.cpp',
  'src/framework/kgr/net/SlaveServer.cpp',
  'src/framework/namespace/Directory.cpp',
  'src/framework/namespace/Filesystem.cpp',
  'src/framework/namespace/Find.cpp',
  'src/framework/namespace/Mount.cpp',
  'src/framework/namespace/Object.cpp',
  'src/framework/namespace/Path.cpp',
  'src/framework/namespace/Resolve.cpp',
  'src/framework/namespace/View.cpp',
  'src/framework/namespace/Virtual.cpp',
  'src/framework/net/CryptoSocket.cpp',
  'src/framework/net/Socket.cpp',
  'src/framework/sched/EventLoop.cpp',
  'src/framework/sched/Scheduler.cpp',
  'src/framework/screen/Component.cpp',
  'src/framework/screen/ComponentTree.cpp',
  'src/framework/screen/Splitter.cpp',
  'src/framework/screen/terminal/TerminalResize.cpp',
  'src/framework/screen/terminal/TerminalSize.cpp',
  'src/framework/screen/terminal/multiplexer/TerminalWindow.cpp',
  'src/framework/screen/terminal/multiplexer/TerminalBuffer.cpp',
  'src/framework/screen/terminal/multiplexer/TerminalSplitter.cpp',
  'src/framework/screen/terminal/multiplexer/TerminalTabber.cpp',
  'src/framework/screen/terminal/components/ComponentHandle.cpp',
  'src/framework/screen/terminal/components/MultiplexerComponent.cpp',
  'src/framework/screen/terminal/components/SplitterComponent.cpp',
  'src/framework/screen/terminal/components/TabberComponent.cpp',
  'src/framework/screen/terminal/components/TextPane.cpp',
  'src/framework/screen/terminal/components/TextPaneComponent.cpp',
  'src/framework/screen/widgets/TextEditor.cpp',
  'src/framework/security/Authenticator.cpp',
  'src/framework/security/Master.cpp',
  'src/framework/security/Restriction.cpp',
  'src/framework/security/Slave.cpp',
  'src/framework/services/ScreenInput.cpp',
  'src/framework/services/ScreenSize.cpp',
  'src/framework/services/Cursor.cpp',
  'src/framework/services/DocumentNotify.cpp',
  'src/framework/services/DocumentSet.cpp',
  'src/framework/services/Namespace.cpp',
  'src/framework/services/Screen.cpp',
  'src/framework/services/Search.cpp',
  'src/framework/services/Service.cpp',
  'src/framework/services/TextPane.cpp',
  'src/framework/services/TextRender.cpp',
  'src/framework/text/TextBlock.cpp',
  'src/framework/text/TextBlockHandle.cpp',
  'src/framework/text/TextChunk.cpp',
  'src/framework/text/TextDocument.cpp',
  'src/framework/text/TextFrame.cpp',
  'src/framework/text/TextRegion.cpp',
  'src/framework/text/TextView.cpp',
  'src/framework/text/cursor/Cursor.cpp',
  'src/framework/text/cursor/EditingPrimitives.cpp',
  'src/framework/text/cursor/Transaction.cpp',
  'src/framework/text/cursor/TransactionBatch.cpp',
  'src/framework/text/cursor/TransactionCursor.cpp',
  'src/framework/text/cursor/TransactionFactory.cpp',
  'src/framework/text/cursor/TransactionStreamMultiplexer.cpp',
  'src/framework/text/search/Match.cpp',
  'src/framework/text/search/Replace.cpp',
]

posix_platform_src = [
  'src/platform/posix/core/awaitable/Posix.cpp',
  'src/platform/posix/filesystem/posix/File.cpp',
  'src/platform/posix/filesystem/posix/FileIO.cpp',
  'src/platform/posix/filesystem/posix/Reader.cpp',
  'src/platform/posix/filesystem/posix/View.cpp',
  'src/platform/posix/filesystem/posix/Writer.cpp',
  'src/platform/posix/namespace/posix/Environment.cpp',
  'src/platform/posix/namespace/posix/Filesystem.cpp',
  'src/platform/posix/net/PosixSocket.cpp',
  'src/platform/posix/screen/terminal/posix/PosixTerminal.cpp'
]

botan_crypto_src = [
  'src/crypto/botan/Botan.cpp'
]

lua_bindings_src = [
  'src/script/lua/Common.cpp',
  'src/script/lua/Lua.cpp',
  'src/script/lua/Logger.cpp',
  'src/script/lua/Pipe.cpp',
  'src/script/lua/Sched.cpp',
  'src/script/lua/Server.cpp'
]

demo_src = [
  'src/demo/main.cpp'
]

cpp = meson.get_compiler('cpp')

threads = dependency('threads')
sloked_framework_lib = static_library('sloked', sloked_src,
  include_directories : include_directories('headers/framework'),
  dependencies : [threads])
sloked_framework = declare_dependency(link_with : sloked_framework_lib,
  include_directories : include_directories('headers/framework'))

ncurses = dependency('ncurses')
sloked_posix_platform_lib = static_library('sloked_posix_platform', posix_platform_src,
  include_directories : include_directories('headers/platform/posix'),
  dependencies : [ncurses, sloked_framework])
sloked_posix_platform = declare_dependency(link_with : sloked_posix_platform_lib,
  include_directories : include_directories('headers/platform/posix'))

botan = dependency('botan-2')
sloked_botan_crypto_lib = static_library('sloked_crypto', botan_crypto_src,
  include_directories : include_directories('headers/crypto/botan'),
  dependencies : [botan, sloked_framework])
sloked_botan_crypto = declare_dependency(link_with : sloked_botan_crypto_lib,
  include_directories : include_directories('headers/crypto/botan'))

lua = dependency('lua')
sloked_lua_bindings_lib = static_library('sloked_lua', lua_bindings_src,
  include_directories : include_directories('headers/script/lua'),
  dependencies : [lua, sloked_framework])
sloked_lua_bindings = declare_dependency(link_with : sloked_lua_bindings_lib,
  include_directories : include_directories('headers/script/lua'))

demo_editor = executable('sloked', demo_src,
  dependencies : [sloked_framework, sloked_posix_platform, sloked_botan_crypto, sloked_lua_bindings])