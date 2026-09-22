English | [中文](docs/README/README.zh-CN.md)

# Bind — Bind Integrates Nested Data

A lightweight relational database built from scratch in C++23, featuring a custom B+ tree storage engine, recursive descent SQL parser, and TCP network protocol.[learn more](docs/guide/guide.md)

## Features

- **Custom Storage Engine**: B+ tree index + fixed-length record files, 4KB pages, full disk persistence
- **Full SQL Support**: DDL (CREATE/DROP/USE) + DML (SELECT/INSERT/UPDATE/DELETE) with recursive descent parser
- **Primary Key Uniqueness**: Enforced via B+ tree index
- **Client-Server Architecture**: Length-prefixed frame protocol over TCP, JSON message format
- **Zero Third-Party Dependencies**: Only C++ standard library + Windows API; custom implementations of ArrayList, LinkedList, String, Json, etc.

## Quick Start

Download pre-built binaries from [Releases](https://gitee.com/kabalts/Bind-for-windows/releases), or build from source.

### Requirements

| Component | Version |
|-----------|---------|
| OS | Windows 11 |
| Compiler | MSVC (Visual Studio 2026) |
| Standard | C++23 |
| Build Tools | CMake 3.15+ |

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
                                    │   (index)         │ │   (data)          │
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
├── include/
│   ├── common/                  # ArrayList, LinkedList, String, Json, ResultSet
│   ├── storage/                 # BPlusTree, FileEngine, Table, Database
│   ├── parser/                  # SQLParser
│   ├── executor/                # Executor
│   ├── network/                 # TcpSocket
│   ├── server/                  # Server
│   └── client/                  # Client
├── src/                         # implementation files
```

## License

[MIT](LICENSE) © kabalts
