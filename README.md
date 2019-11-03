## Sloked - SLOw junKy EDitor

This repo contains components for extensible programming-oriented text editor written from scratch. All implemented subsystems are integrated into a simple demo-editor in `src/main.cpp`. Currently project consists of:
* text management subsystem - stores text in the multiple layers of AVL trees. Bottom layer manages separate lines and larger text chunks (which are split into separate lines dynamically), top layer manages large text parts which are `mmap`-ed in memory and loaded on-demand. This enables relatively efficent navigation and editing of large text files. Multiple encodings are supported (currently UTF-8 and UTF-32LE), others may be added later.
* transaction-based editing which allows simultaneous multiple RW-cursors over a single document with proper (per-cursor) undo-redo semantics.
* text fragmentation mechanism provides the base for syntax highlighting and other high-level transformations (e.g. block folding) of source code.
* VFS-like abstraction which includes nested namespaces, filesystem directory (currently only POSIX paths are supported) mounting and simplified file abstraction. This enabled complex workspace construction and simplifies internal resource management.
* Terminal multiplexer (currently only POSIX terminals are supported) which provides abstractions for multi-window environment, screen splitting, tabs. These abstractions may be directly implemented in other environments (e.g. some graphical toolkit) which simplifies changes in editor UI.
* Message-based service system, which exposes editor functionality through indirect service calls and interconnects editor subsystems. Message system is trasparently availanle through the network, thus allowing editor components (user interface, core library, plugins, remote users) to run on different hosts. The increases flexibility and extensibility, e.g. remote development, collaborative editing, remote plugin execution, plugins on different programming languages.
* Demo-editor `src/demo/main.cpp` which incorporates above-mentioned features into a simplified program used for demonstration and testing purposes.

Some concepts which will be implemented later:
* Implement LSP client and integrate it into existing text fragmentation mechanisms.
* Extend VFS structure by other node types and use it to represent all editor resources in the uniform way.
* Extend existing implementation in practical sense - implement more text encodings, add LSP-based syntax highlighting and hinting.
* Optimize some algorithms in the core of editor.
* Make editor subsystems asynchronous to improve UX of different users/plugins.

### Why?

The main goal is powerful text editing framework which provides high-level abstractions for text editing, sophisticated workspaces, syntax highlighting and hinting, as well as uniform protocols to work with editor subsystems. Exact UI and UX should be provided by external plugins which combine framework components. Networked nature of differect plugins (written in different languages) and even users (with strict permission control) is highly emphasized.

### inb4

* I've published unfinished project as I'm not sure when I'll get a time to continue it. I'll try to continue to work on it, however significant changes in my life make unclear, when and if I could continue.
* Tests? Documentation? Ain't nobody got time for that. I've tried to implement as much functionality as I could in a 1.5 months. No proper tests and docs for now. My bad.

### Author & License

Author: Jevgēnijs Protopopovs

Licese: GNU LGPLv3, see `COPYING` and `COPYING.LESSER`.