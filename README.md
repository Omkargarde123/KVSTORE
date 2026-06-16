# 🗄️ KVStore — Distributed Key-Value Store in C++

A high-performance, multi-threaded, in-memory key-value store built from scratch in C++17.  
Inspired by Redis. Supports **LRU eviction**, **TTL expiry**, **concurrent clients**, and **TCP networking**.

---

## ✨ Features

| Feature | Implementation |
|---|---|
| GET / SET / DEL commands | Custom TCP protocol parser |
| LRU Cache Eviction | Doubly Linked List + HashMap O(1) |
| TTL-based Key Expiry | Min-Heap (Priority Queue) |
| Concurrent Clients | Thread Pool (8 worker threads) |
| TCP Server | POSIX sockets, non-blocking accept loop |
| Graceful Shutdown | SIGINT / SIGTERM signal handling |

---

## 🚀 Quick Start

### Option 1 — Build Locally

**Requirements:** g++ 11+, make, Linux/macOS

```bash
git clone https://github.com/yourusername/kvstore
cd kvstore
make all          # builds server + CLI client
make test         # run test suite
make run          # start server on port 6379
```

### Option 2 — Docker (Recommended)

```bash
docker compose up --build
```

Server starts on port **6379**.

---

## 💻 Usage

**Connect with built-in CLI:**
```bash
./kvstore-cli 127.0.0.1 6379
```

**Or use netcat:**
```bash
nc 127.0.0.1 6379
```

---

## 📖 Commands

```
PING                      → PONG
SET name Google           → OK
SET session abc EX 30     → OK  (expires in 30 seconds)
GET name                  → Google
DEL name                  → (integer) 1
KEYS                      → lists all keys
SIZE                      → (integer) N
```

---

## 🏗️ Architecture

```
                    ┌──────────────────────────────────────────┐
Clients             │              KVStore Server               │
                    │                                          │
 telnet ──TCP──►   │   Accept Loop                            │
 netcat ──TCP──►   │       │                                  │
 cli    ──TCP──►   │       ▼                                  │
                    │   Thread Pool (8 workers)                │
                    │       │                                  │
                    │       ▼                                  │
                    │   Command Parser                         │
                    │       │                                  │
                    │       ▼                                  │
                    │   KVStore Engine                         │
                    │   ┌──────────┐  ┌──────────────────┐   │
                    │   │ LRU List │  │ TTL Min-Heap     │   │
                    │   │ HashMap  │  │ (purge thread)   │   │
                    │   └──────────┘  └──────────────────┘   │
                    └──────────────────────────────────────────┘
```

---

## 📂 Project Structure

```
kvstore/
├── include/
│   └── server.h          # All class declarations
├── src/
│   ├── main.cpp          # Entry point, signal handling
│   ├── server.cpp        # TCP server + command parser
│   ├── kv_store.cpp      # LRU + TTL engine
│   └── thread_pool.cpp   # Thread pool implementation
├── client/
│   └── kvstore-cli.cpp   # Interactive CLI client
├── tests/
│   └── test_kvstore.cpp  # Unit + concurrency tests
├── Makefile
├── Dockerfile
└── docker-compose.yml
```

---

## 🧪 Tests

```bash
make test
```

Covers: SET/GET, overwrite, DELETE, LRU eviction, TTL expiry, concurrent access (10 threads × 100 ops).

---

## ⚙️ Configuration

```bash
./kvstore [port] [threads] [lru_capacity]
./kvstore 6379 8 1000     # defaults
./kvstore 7000 16 5000    # custom
```

---

## 📈 Performance

Benchmarked on a 4-core machine:
- **~50,000 SET/GET ops/sec** with 8 threads
- **<2ms average latency** under concurrent load
- Memory-safe under ASAN with 0 leaks

---

## 🛠️ Tech Stack

- **Language:** C++17
- **Networking:** POSIX TCP Sockets
- **Concurrency:** std::thread, std::mutex, condition_variable
- **Data Structures:** unordered_map, list (LRU), priority_queue (TTL)
- **Build:** Makefile + Docker

---

## 📄 License

MIT License — free to use, modify, distribute.
