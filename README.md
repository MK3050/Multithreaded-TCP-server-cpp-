# Multithreaded TCP Server in C++

A low-level networking project built in C++ to explore how real-world servers handle multiple clients concurrently using different I/O and concurrency models.

This project gradually evolves from a basic blocking TCP server into scalable server architectures using:

* Thread-per-client model
* `select()` based I/O multiplexing
* `kqueue()` event-driven architecture

The goal of this project is to deeply understand operating system level networking concepts, socket programming, concurrency, and scalable server design.

---

# Features

## Implemented

* TCP socket communication
* Multiple client support
* Thread-per-client server
* Client broadcasting
* `select()` based server
* `kqueue()` based server
* Dynamic client management
* Graceful client disconnection handling
* Basic message forwarding

---

# Project Structure

```text
Multithreaded-TCP-Server/
│
├── README.md
├── .gitignore
│
├── src/
│   ├── client.cpp
│   ├── thread_server.cpp
│   ├── select_server.cpp
│   └── kqueue_server.cpp
│
├── build/
├── docs/
└── screenshots/
```

---

# Concepts Covered

## 1. Socket Programming

The project uses BSD sockets in C++.

Core APIs used:

* `socket()`
* `bind()`
* `listen()`
* `accept()`
* `send()`
* `recv()`
* `close()`

These system calls form the foundation of TCP networking on Unix-like systems.

---

## 2. Thread-Per-Client Architecture

In the first implementation, every new client connection gets its own thread.

### Flow

```text
Client connects
      ↓
accept()
      ↓
Create thread
      ↓
Thread handles recv/send for that client
```

### Advantages

* Easy to understand
* Simple logic
* Good for learning concurrency

### Limitations

* High memory usage
* Context switching overhead
* Does not scale well for thousands of clients

---

# 3. select() Based Server

The second implementation removes the need for one thread per client.

Instead of blocking on a single client, the server watches multiple sockets simultaneously using `select()`.

### Flow

```text
Store all sockets in fd_set
        ↓
select() waits for activity
        ↓
Process only ready sockets
```

### Advantages

* Single-threaded concurrency
* Lower resource usage
* Better scalability than thread-per-client

### Limitations

* Limited by file descriptor count
* Scans all descriptors every loop
* Performance degrades with large numbers of clients

---

# 4. kqueue() Event-Driven Server

The final implementation uses `kqueue()`, a scalable event notification system available on BSD/macOS.

Unlike `select()`, `kqueue()` only returns active events instead of scanning every socket.

### Flow

```text
Register socket events with kqueue
             ↓
Kernel monitors sockets
             ↓
Receive only active events
             ↓
Handle client activity
```

### Advantages

* Highly scalable
* Efficient event handling
* Used in production-grade systems
* Handles large numbers of connections efficiently

### Why kqueue is Better than select

| Feature             | select()        | kqueue()     |
| ------------------- | --------------- | ------------ |
| Scalability         | Limited         | High         |
| Descriptor Scanning | O(n)            | Event-driven |
| Performance         | Slower at scale | Efficient    |
| Kernel Support      | Older           | Modern       |

---

# How to Build

## Compile Thread Server

```bash
g++ src/thread_server.cpp -o build/thread_server -pthread
```

## Compile select() Server

```bash
g++ src/select_server.cpp -o build/select_server
```

## Compile kqueue() Server

```bash
g++ src/kqueue_server.cpp -o build/kqueue_server
```

## Compile Client

```bash
g++ src/client.cpp -o build/client -pthread
```

---

# Running the Project

## Start Server

Example:

```bash
./build/thread_server
```

or

```bash
./build/select_server
```

or

```bash
./build/kqueue_server
```

## Start Client

Open multiple terminals:

```bash
./build/client
```

---

# Example Workflow

```text
Client A sends message
          ↓
Server receives message
          ↓
Server broadcasts to all connected clients
          ↓
Other clients receive the message
```

---

# Learning Outcomes

This project helped explore:

* Operating system networking fundamentals
* Concurrent programming
* Blocking vs non-blocking I/O
* Event-driven architecture
* Scalability tradeoffs
* TCP communication internals
* Kernel event notification systems

---

# Future Improvements

Possible future enhancements:

* Non-blocking sockets
* epoll() implementation for Linux
* Custom message protocol
* Authentication system
* Private messaging
* Chat rooms
* File transfer
* TLS/SSL encryption
* Thread pool architecture
* Async logging
* Load testing

---

# Technologies Used

* C++
* POSIX Sockets
* Threads (`std::thread`)
* select()
* kqueue()
* macOS/BSD networking APIs

---

# Why This Project Matters

This project demonstrates understanding of:

* Systems programming
* Low-level networking
* Concurrent server design
* Event-driven architectures
* OS-level I/O mechanisms

It is designed as a learning-focused systems project that gradually builds from simple socket communication to scalable server architectures.
