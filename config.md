# Bind — 运行环境与流程

## 系统环境

- **操作系统**: Windows + WSL (Ubuntu 26.04 LTS)
- **编译器**: g++ 15.2.0
- **标准**: C++23
- **构建**: CMake 4.2.3 / make 4.4.1
- **外部依赖**:
  - C++ 标准库：`<cstdio>` `<cstring>` `<cstdlib>` `<cstdint>` `<cstddef>` `<cmath>` `<cassert>` `<cctype>` `<cerrno>` `<algorithm>` `<new>` `<type_traits>` `<utility>` `<memory>` `<iterator>` `<stdexcept>` `<initializer_list>` `<iostream>` `<ostream>` `<string>`
  - POSIX API：`<unistd.h>` `<sys/stat.h>` `<sys/types.h>` `<sys/socket.h>` `<netinet/in.h>` `<arpa/inet.h>` `<netdb.h>` `<dirent.h>` `<fcntl.h>`
  - 无第三方库

## 快速开始

| 操作 | 命令 | 说明 |
|------|------|------|
| 编译 | `bash build.sh` | CMake 配置 + make 构建 |
| 启动 | `bash bind.sh` | 自动启动服务端 (8888) + 客户端 |
| 测试 | `bash test.sh` | 单元测试 + SQL 集成测试 (9888) |

编译产物位于 `build/bin/`：

| 文件 | 用途 |
|------|------|
| `bind_server` | 数据库服务端 |
| `bind_client` | 命令行客户端 |
| `bind_test` | 测试可执行文件 |

重新编译前需要先删掉 `build/` 目录（`build.sh` 会自动做这件事）。

## 启停流程

### 一键启动（推荐）

```bash
bash bind.sh
```

- 自动在后台启动 `bind_server`（端口 8888）
- 启动 `bind_client` 进入交互模式
- Ctrl+C / `exit` 退出客户端，服务端自动关闭

### 手动分别启动

```bash
# 终端1：启动服务端
./build/bin/bind_server -p 8888

# 终端2：启动客户端
./build/bin/bind_client
```

## 自动化测试

```bash
bash test.sh
```

测试分为两阶段：

1. **Phase 1: C++ 单元测试** — 运行 `bind_test`，覆盖 ArrayList、LinkedList、String、Json、BPlusTree、FileEngine 等基础组件
2. **Phase 2: SQL 集成测试** — 临时启动服务端 (9888)，把 `test_samples.sql` 通过管道喂给客户端，验证 DDL / INSERT / SELECT / UPDATE / DELETE 各功能

## 项目结构

```
Bind/
├── CMakeLists.txt
├── config.md
├── build.sh              # 编译脚本
├── bind.sh               # 一键启动脚本
├── test.sh               # 自动化测试脚本
├── test_samples.sql      # SQL 集成测试用例
├── include/
│   ├── common/           # ArrayList, LinkedList, String, Json, ResultSet
│   ├── storage/          # BPlusTree, FileEngine, Table, Database
│   ├── parser/           # SQLParser
│   ├── executor/         # Executor
│   ├── network/          # TcpSocket
│   ├── server/           # Server
│   └── client/           # Client
├── src/                  # 对应实现文件
│   ├── main_server.cpp
│   └── main_client.cpp
└── tests/
    └── test_main.cpp     # C++ 单元测试
```

## 支持的功能

### SQL 语句

| 语句 | 语法 |
|------|------|
| 创建数据库 | `CREATE DATABASE <name>` |
| 删除数据库 | `DROP DATABASE <name>` |
| 切换数据库 | `USE <name>` |
| 创建表 | `CREATE TABLE <name> (<col> <type>, ...)` |
| 删除表 | `DROP TABLE <name>` |
| 查询 | `SELECT <cols>\|* FROM <name> [WHERE <cond>]` |
| 插入 | `INSERT INTO <name> VALUES (...)` |
| 删除 | `DELETE FROM <name> [WHERE <cond>]` |
| 更新 | `UPDATE <name> SET <col> = <val> [WHERE <cond>]` |

### 数据类型

| 类型 | 说明 |
|------|------|
| `int` | 32位整数 |
| `double` | 双精度浮点数 |
| `string` | 字符串 |
| `bool` | 布尔值 |

### WHERE 运算符

`=` `!=` `<>` `<` `<=` `>` `>=`

### 交互特性

- 提示符显示当前数据库：`Bind>` / `Bind/testdb>`
- `DROP DATABASE` / `DROP TABLE` 有确认提示（仅交互模式）
- `exit` / `quit` 退出客户端
- `help` 查看帮助

## 通信协议

长度前缀帧协议：4 字节大端序长度头 + JSON 正文。

请求：
```json
{"command":"sql", "sql":"select * from users", "db":"mydb"}
```

响应（成功）：
```json
{"status":"ok", "result":{"columns":["id","name"],"rows":[[1,"Alice"]]}, "message":"..."}
```

响应（失败）：
```json
{"status":"error", "message":"Table not found: users"}
```

## 数据存储

- 默认目录：`data/`
- 每个数据库一个子目录：`data/<dbname>/`
- 每个表三个文件：
  - `<table>.dat` — 定长记录（4B id + 1B 删除标记 + 256B 字符串 = 261 字节/条）
  - `<table>.idx` — B+ 树索引（4KB 页大小）
  - `<table>.meta` — 表结构元数据（列定义）

## 端口

| 场景 | 端口 |
|------|------|
| 正常使用 (`bind.sh`) | 8888 |
| 自动化测试 (`test.sh`) | 9888 |
