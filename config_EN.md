# Bind — Environment & Workflow

## System Requirements

- **OS**: Windows 11
- **Compiler**: MSVC (Visual Studio 2026)
- **Standard**: C++23
- **Build**: CMake 3.15+
- **Dependencies**:
  - C++ Standard Library: `<cstdio>` `<cstring>` `<cstdlib>` `<cstdint>` `<cstddef>` `<cmath>` `<cassert>` `<cctype>` `<cerrno>` `<algorithm>` `<new>` `<type_traits>` `<utility>` `<memory>` `<iterator>` `<stdexcept>` `<initializer_list>` `<iostream>` `<ostream>` `<string>`
  - Windows API: `<winsock2.h>` `<ws2tcpip.h>` `<windows.h>`
  - No third-party libraries

## Quick Start

| Action | Command | Description |
|--------|---------|-------------|
| Build | VS: **Build → Build All**, or `cmake --build` | CMake configure + MSBuild |
| Start | `.\bind.ps1` | Auto-start server (8888) + client |

Build outputs in `out/build/x64-Debug/bin/`:

| File | Purpose |
|------|---------|
| `bind_server.exe` | Database server |
| `bind_client.exe` | CLI client |

Clean the `out/` directory before rebuilding.

## Startup

### One-click start

```powershell
.\bind.ps1
```

- Starts `bind_server.exe` (port 8888) and `bind_client.exe`
- Server window stays running, client window enters interactive mode
- Ctrl+C / `exit` to quit the client, close the server window to stop

### Manual start

```powershell
# Terminal 1: start server
.\out\build\x64-Debug\bin\bind_server.exe -p 8888

# Terminal 2: start client
.\out\build\x64-Debug\bin\bind_client.exe
```

## Project Structure

```
Bind/
├── CMakeLists.txt
├── config.md
├── bind.ps1               # one-click start script
├── include/
│   ├── common/            # ArrayList, LinkedList, String, Json, ResultSet
│   ├── storage/           # BPlusTree, FileEngine, Table, Database
│   ├── parser/            # SQLParser
│   ├── executor/          # Executor
│   ├── network/           # TcpSocket
│   ├── server/            # Server
│   └── client/            # Client
├── src/                   # implementation files
│   ├── main_server.cpp
│   └── main_client.cpp
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
| Normal use (`bind.ps1`) | 8888 |
