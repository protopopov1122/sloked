## Sloked - text editor framework

This repo contains components for extensible programming-oriented text editor written from scratch. All implemented subsystems are integrated into a simple demo-editor in `components/main.cpp`. Currently project consists of:
* text management subsystem - stores text in the multiple layers of AVL trees. Bottom layer manages separate lines and larger text chunks (which are split into separate lines dynamically), top layer manages large text parts which are `mmap`-ed in memory and loaded on-demand. This enables relatively efficent navigation and editing of large text files. Multiple end-of-line (CRLF and LF) encodings are supported (currently UTF-8 and UTF-32LE), others may be added later. Find-replace capabilities.
* transaction-based editing which allows simultaneous multiple RW-cursors over a single document with proper (per-cursor) undo-redo semantics.
* text fragmentation mechanism provides the base for syntax highlighting and other high-level transformations (e.g. block folding) of source code.
* VFS-like abstraction which includes nested namespaces, filesystem directory (currently only POSIX paths are supported) mounting and simplified file abstraction. This enabled complex workspace construction and simplifies internal resource management.
* Terminal multiplexer (currently only POSIX terminals are supported) which provides abstractions for multi-window environment, screen splitting, tabs. These abstractions may be directly implemented in other environments (e.g. some graphical toolkit) which simplifies changes in editor UI.
* Graphical terminal implementation on Cairo + SDL software rendering - needs optimization.
* Message-based service system, which exposes editor functionality through indirect service calls and interconnects editor subsystems. Message system is trasparently available through the network, thus allowing editor components (user  interface, core library, plugins, remote users) to run on different hosts. The increases flexibility and extensibility, e.g. remote development, collaborative editing, remote plugin execution, plugins on different programming languages. Network channels can be encrypted and support per-user encryption and permission model.
* Bindings to Lua which make use of messaging mechanism and provide base for asynchronous Lua plugins.
* Utility code for CLI argument, configuration file and generic JSON parsing, logging, networking, multi-threaded helpers and infrastructure for asynchronous code execution.
* Demo-editor `components/frontend` which incorporates above-mentioned features into a simplified program used for demonstration and testing purposes.

Some concepts which will be implemented later:
* Implement LSP client and integrate it into existing text fragmentation mechanisms.
* Extend existing implementation in practical sense - implement more text encodings, add LSP-based syntax highlighting and hinting.
* Optimize some algorithms in the core of editor.
* **Implement the editor itself**, because currently it's only framework + very basic editing app.


### Why?

The main goal is powerful text editing framework which provides high-level abstractions for text editing, sophisticated workspaces, syntax highlighting and hinting, as well as uniform protocols to work with editor subsystems. Exact UI and UX should be provided by external plugins which combine framework components. Networked nature of differect plugins (written in different languages) and even users (with strict permission control) is highly emphasized.

### Building
Project is being built and tested on Linux x86_64 and ported to FreeBSD, although most of the code is system agnostic (and should be easily portable across POSIX-compatible systems). Project uses `meson` for build system, C++17 as a language, external dependencies are ncurses, Lua 5.3 (optional), Botan (optional).

### inb4

* The project is unfinished. I continue expanding editing framework.
* Tests? Documentation? Ain't nobody got time for that. I tried to implement the whole framework in
one pass with only basic testing, because of lack of time.

### Author & License

Author: Jevgēnijs Protopopovs

Licese: GNU LGPLv3, see `COPYING` and `COPYING.LESSER`.
