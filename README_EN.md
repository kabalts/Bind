English | [中文](README.md)

# Bind — Bind Integrates Nested Data

A lightweight relational database built from scratch in C++23, featuring a custom B+ tree storage engine, recursive descent SQL parser, and TCP network protocol.

## Features

- **Custom Storage Engine**: B+ tree index + fixed-length record files, 4KB pages, full disk persistence
- **Full SQL Support**: DDL (CREATE/DROP/USE) + DML (SELECT/INSERT/UPDATE/DELETE) with recursive descent parser
- **Primary Key Uniqueness**: Enforced via B+ tree index
- **Client-Server Architecture**: Length-prefixed frame protocol over TCP, JSON message format
- **Zero Third-Party Dependencies**: Only C++ standard library + POSIX API; custom implementations of ArrayList, LinkedList, String, Json, etc.

## Quick Start

```bash
# Build
bash build.sh

# Start (server on 8888 + client)
bash bind.sh

# Run tests
bash test.sh
```

### Requirements

| Component | Version |
|-----------|---------|
| OS | Ubuntu 26.04 LTS (WSL) |
| Compiler | g++ 15.2.0 |
| Standard | C++23 |
| Build Tools | CMake 4.2.3 / make 4.4.1 |

## Build

### Install dependencies

```bash
sudo apt install g++ cmake make
```

### Option 1: Build script (recommended)

```bash
bash build.sh
```

### Option 2: Manual build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
make -j$(nproc)
```

### Build outputs

| File | Description |
|------|-------------|
| `build/bin/bind_server` | Database server |
| `build/bin/bind_client` | CLI client |
| `build/bin/bind_test` | Test program |

## Supported SQL

```sql
-- Database operations
CREATE DATABASE mydb;
USE mydb;
DROP DATABASE mydb;

-- Table operations
CREATE TABLE users (id int primary, name string, age int);
DROP TABLE users;

-- CRUD
INSERT INTO users VALUES (1, 'Alice', 25);
SELECT * FROM users WHERE age > 20;
SELECT id, name FROM users;
UPDATE users SET age = 26 WHERE id = 1;
DELETE FROM users WHERE id = 1;
```

### Data Types

`int` `double` `string` `bool`

### WHERE Operators

`=` `!=` `<>` `<` `<=` `>` `>=`

## Architecture

```
┌──────────────┐     TCP (length-prefix + JSON)    ┌──────────────┐
│  Client      │ ◄────────────────────────────────►│  Server      │
│ (bind_client)│                                   │ (bind_server)│
└──────────────┘                                   └──────┬───────┘
                                                          │
                                              ┌───────────┴───────────┐
                                              │       Executor        │
                                              └───────────┬───────────┘
                                                          │
                                              ┌───────────┴───────────┐
                                              │      SQLParser        │
                                              └───────────┬───────────┘
                                                          │
                                     ┌────────────────────┴────────────────────┐
                                     │                  Database               │
                                     └────────────────────┬────────────────────┘
                                                          │
                                     ┌────────────────────┴────────────────────┐
                                     │                   Table                 │
                                     └──────────┬─────────────────┬────────────┘
                                                │                 │
                                    ┌───────────┴───────┐ ┌───────┴───────────┐
                                    │    BPlusTree      │ │    FileEngine     │
                                    │   (索引持久化)     │ │   (数据持久化)     │
                                    └───────────────────┘ └───────────────────┘
```

## Data Storage

```
data/
└── mydb/                  # one directory per database
    ├── users.dat          # fixed-length records: 261 bytes/row
    ├── users.idx          # B+ tree index: 4KB/page
    └── users.meta         # table schema metadata
```

## Protocol

4-byte big-endian length header + JSON payload.

Request:
```json
{"command": "sql", "sql": "select * from users", "db": "mydb"}
```

Response:
```json
{"status": "ok", "result": {"columns": ["id", "name"], "rows": [[1, "Alice"]]}}
```

## Project Structure

```
Bind/
├── CMakeLists.txt
├── build.sh                    # build script
├── bind.sh                     # one-click start
├── test.sh                     # automated test suite
├── test_samples.sql            # SQL integration test cases
├── include/
│   ├── common/                 # ArrayList, LinkedList, String, Json, ResultSet
│   ├── storage/                # BPlusTree, FileEngine, Table, Database
│   ├── parser/                 # SQLParser
│   ├── executor/               # Executor
│   ├── network/                # TcpSocket
│   ├── server/                 # Server
│   └── client/                 # Client
├── src/                        # implementation files
└── tests/
    └── test_main.cpp           # C++ unit tests
```

## License

[MIT](LICENSE) © kabalts
