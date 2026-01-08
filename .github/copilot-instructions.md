## Copilot Instructions

**Project overview**
- Reactor-style event loop skeleton in C++11 with manual allocation; entry point currently empty in [src/main.cpp](src/main.cpp#L1-L6).
- Builds a single `main` executable from all sources, outputting to `output/main`; see [CMakeLists.txt](CMakeLists.txt#L1-L11) for flags and the pthread link.
- Typical build from the `build/` dir: `cmake .. && make`; VS Code tasks named `cmake` and `Build my project` chain these steps.

**Core primitives**
- Event flags `TimeOutEvent|READ_EVENT|WRITE_EVENT` and handler signatures live in [include/Channel.h](include/Channel.h#L5-L23); `ChannelInit` allocates with `malloc` and stores handlers/arg in [src/Channel.cpp](src/Channel.cpp#L3-L12).
- `writeEventEnable` toggles the `WRITE_EVENT` bit and `isWriteEventEnable` checks it in [src/Channel.cpp](src/Channel.cpp#L14-L29).
- `ChannelMap` holds an array of `Channel*`; `ChannelMapInit` allocates but does not zero `list`, so callers should initialize slots before use [src/ChannelMap.cpp](src/ChannelMap.cpp#L3-L9).
- `makeMapRoom` doubles capacity until `newSize`, reallocates, and zero-fills the newly added range [src/ChannelMap.cpp](src/ChannelMap.cpp#L29-L50); `ChannelMapClear` frees any heap-allocated channels and the array [src/ChannelMap.cpp](src/ChannelMap.cpp#L11-L27).

**Event loop and dispatcher shape**
- `EventLoop` currently only carries a `Dispatcher*` plus opaque `dispatcherData`; intended default dispatcher is `Epoll_dispatcher` [include/EventLoop.h](include/EventLoop.h#L4-L9).
- `Dispatcher` is a function-pointer vtable for `init/add/remove/modify/dispatch/clear` against `Channel*` and the loop [include/Dispatcher.h](include/Dispatcher.h#L7-L21).

**Epoll dispatcher status**
- `Epoll_dispatcher` is declared with all function pointers but bodies are stubs in [src/EpollDispatcher.c](src/EpollDispatcher.c#L1-L62); only `epollinit` creates an epoll fd and currently returns nothing.
- `epoll_eventData` (epfd + `struct epoll_event*`) is the intended `dispatcherData`; implementations should populate it, register/unregister fds, dispatch with `epoll_wait`, and clean up epfd/event buffer in `epollclear`.

**Work in progress / gaps**
- No event loop driving logic yet; `main` is empty and `test` is a stub [src/test.cpp](src/test.cpp#L1-L4).
- No error handling or logging is present; most functions assume valid pointers and heap allocations.

**Conventions and cautions**
- Code mixes C and C++; headers use `using namespace std;` and C-style allocation; stay consistent (avoid RAII until the design changes).
- Manual memory management: free channels placed in `ChannelMap` before `ChannelMapClear`, and ensure new map slots are zeroed before dereference.
- Keep new APIs compatible with the function-pointer dispatcher style; prefer extending existing structs over introducing classes unless refactoring wholesale.

**Domain note**
- [notebook.md](notebook.md#L2-L21) contains a captured HTTP GET request used for parsing examples; useful when adding real handlers.
