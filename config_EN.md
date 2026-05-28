# Bind — Environment & Workflow

## System Requirements

- **OS**: Windows + WSL (Ubuntu 26.04 LTS)
- **Compiler**: g++ 15.2.0
- **Standard**: C++23
- **Build**: CMake 4.2.3 / make 4.4.1
- **Dependencies**:
  - C++ Standard Library: `<cstdio>` `<cstring>` `<cstdlib>` `<cstdint>` `<cstddef>` `<cmath>` `<cassert>` `<cctype>` `<cerrno>` `<algorithm>` `<new>` `<type_traits>` `<utility>` `<memory>` `<iterator>` `<stdexcept>` `<initializer_list>` `<iostream>` `<ostream>` `<string>`
  - POSIX API: `<unistd.h>` `<sys/stat.h>` `<sys/types.h>` `<sys/socket.h>` `<netinet/in.h>` `<arpa/inet.h>` `<netdb.h>` `<dirent.h>` `<fcntl.h>`
  - No third-party libraries

## Quick Start

| Action | Command | Description |
|--------|---------|-------------|
| Build | `bash build.sh` | CMake configure + make build |
| Start | `bash bind.sh` | Auto-start server (8888) + client |
| Test | `bash test.sh` | Unit tests + SQL integration tests (9888) |

Build outputs in `build/bin/`:

| File | Purpose |
|------|---------|
| `bind_server` | Database server |
| `bind_client` | CLI client |
| `bind_test` | Test executable |

Rebuilding requires removing the `build/` directory first (`build.sh` does this automatically).

## Startup

### One-click start (recommended)

```bash
bash bind.sh
```

- Starts `bind_server` in background (port 8888)
- Launches `bind_client` in interactive mode
- Ctrl+C / `exit` to quit the client, server shuts down automatically

### Manual start

```bash
# Terminal 1: start server
./build/bin/bind_server -p 8888

# Terminal 2: start client
./build/bin/bind_client
```

## Automated Testing

```bash
bash test.sh
```

Two phases:

1. **Phase 1: C++ Unit Tests** — runs `bind_test`, covering ArrayList, LinkedList, String, Json, BPlusTree, FileEngine, and other core components
2. **Phase 2: SQL Integration Tests** — starts a temporary server (9888), pipes `test_samples.sql` to the client, verifying DDL / INSERT / SELECT / UPDATE / DELETE functionality

## Project Structure

```
Bind/
├── CMakeLists.txt
├── config.md
├── build.sh              # build script
├── bind.sh               # one-click start script
├── test.sh               # automated test script
├── test_samples.sql      # SQL integration test cases
├── include/
│   ├── common/           # ArrayList, LinkedList, String, Json, ResultSet
│   ├── storage/          # BPlusTree, FileEngine, Table, Database
│   ├── parser/           # SQLParser
│   ├── executor/         # Executor
│   ├── network/          # TcpSocket
│   ├── server/           # Server
│   └── client/           # Client
├── src/                  # implementation files
│   ├── main_server.cpp
│   └── main_client.cpp
└── tests/
    └── test_main.cpp     # C++ unit tests
```

## Supported Features

### SQL Statements

| Statement | Syntax |
|-----------|--------|
| Create Database | `CREATE DATABASE <name>` |
| Drop Database | `DROP DATABASE <name>` |
| Switch Database | `USE <name>` |
| Create Table | `CREATE TABLE <name> (<col> <type>, ...)` |
| Drop Table | `DROP TABLE <name>` |
| Query | `SELECT <cols>\|* FROM <name> [WHERE <cond>]` |
| Insert | `INSERT INTO <name> VALUES (...)` |
| Delete | `DELETE FROM <name> [WHERE <cond>]` |
| Update | `UPDATE <name> SET <col> = <val> [WHERE <cond>]` |

### Data Types

| Type | Description |
|------|-------------|
| `int` | 32-bit integer |
| `double` | Double-precision float |
| `string` | String |
| `bool` | Boolean |

### WHERE Operators

`=` `!=` `<>` `<` `<=` `>` `>=`

### Interactive Features

- Prompt shows current database: `Bind>` / `Bind/testdb>`
- `DROP DATABASE` / `DROP TABLE` confirmation prompt (interactive mode only)
- `exit` / `quit` to exit the client
- `help` to view help

## Protocol

Length-prefixed frame protocol: 4-byte big-endian header + JSON payload.

Request:
```json
{"command":"sql", "sql":"select * from users", "db":"mydb"}
```

Response (success):
```json
{"status":"ok", "result":{"columns":["id","name"],"rows":[[1,"Alice"]]}, "message":"..."}
```

Response (failure):
```json
{"status":"error", "message":"Table not found: users"}
```

## Data Storage

- Default directory: `data/`
- One subdirectory per database: `data/<dbname>/`
- Three files per table:
  - `<table>.dat` — fixed-length records (4B id + 1B deleted flag + 256B string = 261 bytes/row)
  - `<table>.idx` — B+ tree index (4KB pages)
  - `<table>.meta` — table schema metadata (column definitions)

## Ports

| Scenario | Port |
|----------|------|
| Normal use (`bind.sh`) | 8888 |
| Automated testing (`test.sh`) | 9888 |
