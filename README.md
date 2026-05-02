# Multithreaded TCP Chat Server in C++

> Scalable TCP chat server built from scratch in C++ using BSD sockets and an event-driven architecture with kqueue — a deep systems programming exercise in how real networking servers work internally.

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
![Platform](https://img.shields.io/badge/platform-macOS%20%2F%20BSD-orange)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Overview

This project was built as a deep systems programming exercise to understand how real networking servers work internally. The final implementation uses kqueue with non-blocking sockets for a scalable, single-threaded, event-driven design.

---

## Project Evolution

The server was built iteratively across three stages, each addressing the scalability limitations of the previous approach:

| Stage | Approach | Notes |
|-------|----------|-------|
| 1 | Thread-per-client | Simple but doesn't scale |
| 2 | `select()`-based | Better, but limited by `fd_set` size |
| **3** | **kqueue event-driven** | **Current — scalable, non-blocking** |

---

## Architecture

```
Client(s)
    ↓
TCP Socket
    ↓
kqueue Event Loop
    ↓
Connection Management
    ↓
Protocol Parsing
    ↓
Broadcast System
```

---

## Features

- TCP socket programming using BSD sockets
- Non-blocking sockets (`fcntl` + `O_NONBLOCK`)
- Event-driven architecture using kqueue
- Multiple concurrent clients
- Length-prefixed message framing
- Incremental stream parsing
- Per-connection input/output buffers
- Backpressure-aware non-blocking writes
- Username support (`/name` command)
- Modular server architecture

---

## Key Networking Concepts

### TCP Is a Byte Stream

TCP does not preserve message boundaries. A single `send()` may arrive as:

- Multiple `recv()` calls
- Partial data
- Merged messages

This project solves that using length-prefixed framing.

### Message Framing

Every message sent over the wire follows this format:

```
[length][message body]
```

Example:

```
[0005]hello
```

This allows the server to reconstruct complete messages regardless of how TCP splits them.

### Non-Blocking I/O

All sockets are configured using:

```cpp
fcntl(fd, F_SETFL, O_NONBLOCK)
```

The server never blocks waiting for a client.

### Event-Driven Design

The server uses `kqueue` to monitor:

- Readable sockets
- Writable sockets
- New connections

This allows a single thread to efficiently handle many clients.

### Backpressure Handling

Non-blocking `send()` may only transmit partial data. To handle this:

- Each client has a dedicated output buffer
- Unsent data remains queued
- `EVFILT_WRITE` notifications resume sending when the socket is ready

---

## Folder Structure

```
src/
├── client/
│   └── client.cpp
│
├── server/
│   └── main.cpp
│
├── common/
│   ├── protocol.cpp
│   ├── protocol.hpp
│   ├── socket_utils.cpp
│   └── socket_utils.hpp
│
└── core/
    ├── Connection.cpp
    ├── Connection.hpp
    ├── Server.cpp
    └── Server.hpp
```

---

## Build & Run

### Create build directory

```bash
mkdir build
```

### Build server

```bash
make server
```

### Build client

```bash
make client
```

### Start server

```bash
./build/server
```

### Run client

```bash
./build/client
```

Open multiple client terminals to test chat broadcasting.

---

## Username & Chat

Set your username:

```
/name alice
```

Messages will appear as:

```
[alice]: hello everyone
[bob]: hey alice!
```

---

## Technologies

| Technology | Role |
|------------|------|
| C++17 | Language |
| BSD sockets | Raw TCP networking |
| kqueue | Kernel I/O multiplexing |
| POSIX APIs | System-level calls |
| Non-blocking I/O | Scalable connection handling |
| Message framing | Reliable protocol over TCP streams |

---

## Future Improvements

- [ ] Chat rooms
- [ ] Private messaging
- [ ] Authentication
- [ ] TLS encryption
- [ ] Logging system
- [ ] Graceful shutdown
- [ ] Configuration files
- [ ] Benchmarks / load testing

---

## Learning Goals

This project was built not just to create a chat server, but to deeply understand:

- How TCP works internally
- Why message framing is required
- How scalable event-driven servers are designed
- How real systems handle partial reads and writes
- The tradeoffs between threads and event loops

---

## License

MIT License — free to use, modify, and distribute.