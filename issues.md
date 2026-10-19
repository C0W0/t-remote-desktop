# Known issues

Last reviewed: 2026-10-12, against `main` at `d406c52`.

Scope: the `network/` library. The programs under `app/` are test programs and only appear where they expose a library problem.

Severity:
- **Medium**: wrong behavior in plausible situations, or a security concern.
- **Low**: cosmetic, a design constraint, or only hit under unusual conditions.

## Open issues

| # | Area | Issue | Where | Severity | Suggested fix |
|---|------|-------|-------|----------|---------------|
| 1 | TCP (POSIX) | `recv()` and `send()` treat `EINTR` as fatal. A signal that interrupts the call makes the connection shut down. | `network/tcp/src/posix/ConnectionSocketPosixImpl.cpp`, `recv()` / `send()` | Medium | Retry the call when `errno == EINTR`, as `Accept` already does. |
| 2 | Protocol | `deserializeHeader` does not validate `MessageType` (and `ResponseStatus` is never validated). The commented-out check uses `std::views::enum_range`, which does not exist. Unknown values pass through as invalid enum values. | `network/protocol/src/Transport.cpp:43-46` | Medium | Reject unknown values with an explicit range check and return an error. |
| 3 | Protocol | `TransportHeader::length` has no upper bound. A consumer that allocates `length` bytes (the host app does) can be made to allocate up to 4 GiB by an unauthenticated peer. | `network/protocol/include/protocol/Transport.h` (`TransportHeader`), `app/host/src/main.cpp` (`Message` case) | Medium | Define a maximum message length and reject larger headers in `deserializeHeader`. |
| 4 | TCP server | The listener binds all interfaces (`AI_PASSIVE`, no host) but `Server.cpp` logs `Listening on localhost:{}`. Authentication is still a TODO, so this is an exposure. | `network/tcp/src/Server.cpp:15`, both listening-socket implementations | Medium | Bind loopback explicitly or make the interface configurable, and fix the log message. |
| 5 | TCP (POSIX) | The listening socket does not set `SO_REUSEADDR`. Restarting the server while earlier connections are in TIME_WAIT fails with `EADDRINUSE`. Do not add this on Windows, where it has different semantics. | `network/tcp/src/posix/ListeningSocketPosixImpl.cpp`, `Listen()` | Medium | `setsockopt(SO_REUSEADDR)` before `bind` on POSIX only. |
| 6 | TCP server | `TcpServer`'s move constructor and move assignment only move `serverSocket_`. `onAccept_` is dropped, so a moved server loses its handler. | `network/tcp/include/tcp/Server.h:22-27` | Low | Move `onAccept_` as well, or default both operations. |
| 7 | TCP | `recv()` and `send()` dereference a null `pImpl_` on a moved-from socket, and `Accept` does the same with a moved-from listener. Only `close()` / `abort()` are guarded. | `network/tcp/src/Socket.cpp:65`, `:74`, `:86`; the `Accept` implementations | Low | Guard and return an error, or document the precondition. |
| 8 | API | A clean peer close is reported as `std::unexpected(0)`, which shares the error channel with real `errno` / WSA codes. | `recv()` in both connection implementations | Low | A dedicated result or a named constant instead of the bare `0`. |
| 9 | Concurrency (contract) | Only one thread may be in `Accept` at a time, and a socket must not be destroyed while another thread is using it. Neither is enforced. | `network/tcp/include/tcp/Socket.h`, the listening-socket implementations | Low | Document the contract in `Socket.h`. |
| 10 | Concurrency (contract) | `send()` loops over partial writes, so two threads sending on the same socket can interleave bytes on the stream. | `ConnectionSocket::send` in `network/tcp/src/Socket.cpp` | Low | Serialize writers in the layer above, or add a send mutex. |
| 11 | Protocol | The serializers assume the structs have no padding (`sizeof` equals the sum of the field sizes). There is no `static_assert`. It works today only by coincidence of field sizes. | `network/protocol/src/Transport.cpp` | Low | `static_assert` the sizes (8 for the header, 4 for `AuthMeta`, 4 for `ServerRespMeta`). |
| 12 | Portability | `Socket.cpp` selects the Windows implementation with `#ifdef WIN32`. `_WIN32` is the macro the compiler always defines. | `network/tcp/src/Socket.cpp:7` | Low | Use `_WIN32`. |
| 13 | Build | `CMAKE_CXX_STANDARD_REQUIRED` is not set, so a compiler can silently fall back below C++23. | `CMakeLists.txt:4` | Low | `set(CMAKE_CXX_STANDARD_REQUIRED ON)`. |
| 14 | Code quality | Sizes are compared as signed and unsigned, and narrowed to `int` in `send()` and `recv()`. Buffers over 2 GiB are truncated. | `network/tcp/src/Socket.cpp:69`, `:85`; both `send()` / `recv()` implementations | Low | Use `size_t` / `ssize_t` consistently and chunk large buffers. |
| 15 | Code quality | `return std::move(socketImpl)` defeats copy elision (3 places). | `network/tcp/src/windows/ConnectionSocketWinImpl.cpp:74`, `:127`; `ListeningSocketWinImpl.cpp:92` | Low | `return socketImpl;` |
| 16 | Code quality | Stray `;` after the namespace closing brace. | `network/tcp/include/tcp/Server.h:47` | Low | Remove it. |
| 17 | Code quality | The Windows `Connect` sets `AI_PASSIVE`, which has no effect when connecting. | `network/tcp/src/windows/ConnectionSocketWinImpl.cpp`, `Connect()` | Low | Remove the flag. |
| 18 | Logging | The library prints to stdout on every call (for example `Bytes sent: N` for each send), and `shutdown` on an already-reset peer logs an `ENOTCONN` failure. | both connection implementations | Low | Remove, or put behind a log level. |
| 19 | Repo hygiene | `cmake-build-debug/` is not listed in `.gitignore`. `git status` does not show it, so it is presumably covered by a global ignore on this machine. | `.gitignore` | Low | Add it to the repo's `.gitignore`. |
| 20 | Test app | The host ignores the `[[nodiscard]]` result of `send()` when replying `ServerBusy` (compiler warning). | `app/host/main.cpp:132` | Low | Check the result. |

## Test coverage gaps

- There are no tests for `Transport` serialization or deserialization.
- There are no tests for the `EINTR` handling (issue 1) or for the `TcpServer` move behavior (issue 6).
- `ConnectionLifetimeTest.UseAfterCloseNeverTouchesAnotherSocket` can only catch handle reuse on POSIX. On Windows it is a sanity check.
- The Windows listener assumes Winsock re-signals `FD_ACCEPT` after `accept()` while more connections are pending. The tests pass on Windows, but this has not been exercised with a large backlog.
