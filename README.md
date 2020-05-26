## Sloked - text editor framework

This repo contains components for extensible programming-oriented text editor written from scratch. All implemented subsystems are integrated into a simple demo-editor in `components/frontend-cpp` and `components/headless/src/sample` (functional equivalents). Currently project consists of:
* text management subsystem - stores text in the multiple layers of AVL trees. Bottom layer manages separate lines and larger text chunks (which are split into separate lines dynamically), top layer manages large text parts which are `mmap`-ed in memory and loaded on-demand. This enables relatively efficent navigation and editing of large text files. Multiple end-of-line (CRLF and LF) encodings are supported (currently UTF-8 and UTF-32LE), others may be added later. Find-replace capabilities.
* transaction-based editing engine which allows simultaneous multiple RW-cursors over a single document with proper (per-cursor) undo-redo semantics.
* text fragmentation mechanism provides the base for syntax highlighting and other high-level transformations (e.g. block folding) of source code.
* VFS-like abstraction which includes nested namespaces, filesystem directory (Win32 paths are mapped into POSIX) mounting and simplified file abstraction. This enables complex workspace construction and simplifies internal resource management.
* Terminal multiplexer (for both POSIX and Win32) which provides abstractions for multi-window environment, screen splitting, tabs. These abstractions may be directly implemented in other environments (e.g. some graphical toolkit) which simplifies changes in editor UI.
* Graphical terminal implementation on Cairo + SDL software rendering - needs optimization.
* Threading, asynchronous scheduling and processing library used to organise editor internals. 
* Message-based service system, which exposes editor functionality through indirect service calls and interconnects editor subsystems. Message system is trasparently available through the network, thus allowing editor components (user  interface, core library, plugins, remote users) to run on different hosts and use different technologies. The increases flexibility and extensibility, e.g. remote development, collaborative editing, remote plugin execution, plugins on different programming languages. Network channels can be compressed, encrypted and support per-user encryption and permission model.
* Bindings to Lua which make use of messaging mechanism and provide a base for asynchronous Lua plugins.
* Messaging stack implementation in TypeScript for NodeJS platform. Sample editor implementation in TypeScript.
* Utility code for CLI argument, configuration file and generic JSON parsing, logging, networking, encoding and various algorithm implementations.
* Demo-editor `components/frontend` which incorporates above-mentioned features into a simplified program used for demonstration and testing purposes.

Some concepts which will be implemented later:
* Implement LSP client and integrate it into existing text fragmentation mechanisms.
* Extend existing implementation in practical sense - implement more text encodings, add LSP-based syntax highlighting and hinting.
* Optimize some algorithms in the core of editor.
* Extensive testing - currently there's only limited number of unit tests. Integration tests are based on the fact that there exist two implementations of messaging
  stack (C++ and TypeScript) which interoperate extensively. This fact ensures that each implementation by itself is correct enough.
* **Implement the editor itself**, because currently it's only framework + very basic editing app.

### Why?

The main goal is powerful text editing framework which provides high-level abstractions for text editing, sophisticated workspaces, syntax highlighting and hinting, as well as uniform protocols to work with editor subsystems. Exact UI and UX should be provided by external plugins which combine framework components. Networked nature of differemt plugins (written in different languages) and even users (with strict permission control) is highly emphasized.

### Project structure and supported platforms
Project code is split into multiple independent components separated by interface. Core functionality (`framework`) is almost platform-agnostic and depends only on
C++17-compatible compiler. UI implementation and NodeJS bindings also do not depend on any specific platform. Platform specific code is isolated in `platform` component
and currently includes support for POSIX and Win32; project is compiled and tested on Linux (GCC, Clang), FreeBSD (GCC, Clang), Windows (MinGW). All third-party library
interfaces are also separated into distinct components which are optional during the build. Current list of optional dependencies: Botan-2, Zlib, Lua 5.3 (works on both
POSIX and Win32 systems), OpenSSL, SDL2, Cairo, Pango (POSIX systems only). Project uses `meson` build system.

Dependency build procedure will be described later.

### inb4
* The project is unfinished. I continue expanding editing framework.
* Tests? Documentation? Ain't nobody got time for that. I tried to implement the whole framework in
one pass with only basic testing, because of lack of time.

### Author & License

Author: Jevgēnijs Protopopovs

Licese: GNU LGPLv3, see `COPYING` and `COPYING.LESSER`.
