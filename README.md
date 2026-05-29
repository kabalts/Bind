[English](README_EN.md) | 中文

# Bind — Bind Integrates Nested Data

一个用 C++23 从零构建的轻量级关系型数据库，包含自定义 B+ 树存储引擎、递归下降 SQL 解析器和 TCP 网络协议。

## 特性

- **自制存储引擎**：B+ 树索引 + 定长记录文件，4KB 页大小，支持磁盘持久化
- **完整 SQL 支持**：DDL (CREATE/DROP/USE) + DML (SELECT/INSERT/UPDATE/DELETE)，递归下降解析器
- **主键唯一约束**：基于 B+ 树索引进阶保证
- **客户端-服务端架构**：长度前缀帧协议 (TCP)，JSON 格式通信
- **无第三方依赖**：仅使用 C++ 标准库 + POSIX API，自实现 ArrayList、LinkedList、String、Json 等容器

## 快速开始

```bash
# 编译
bash build.sh

# 启动（服务端 8888 + 客户端）
bash bind.sh

# 运行测试
bash test.sh
```

### 环境要求

| 组件     | 版本                     |
| -------- | ------------------------ |
| 操作系统 | Ubuntu 26.04 LTS (WSL)   |
| 编译器   | g++ 15.2.0               |
| 标准     | C++23                    |
| 构建工具 | CMake 4.2.3 / make 4.4.1 |

## 构建

### 安装依赖

```bash
sudo apt install g++ cmake make
```

### 方式一：使用构建脚本（推荐）

```bash
bash build.sh
```

### 方式二：手动构建

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
make -j$(nproc)
```

### 编译产物

| 文件                      | 说明         |
| ------------------------- | ------------ |
| `build/bin/bind_server` | 数据库服务端 |
| `build/bin/bind_client` | 命令行客户端 |
| `build/bin/bind_test`   | 测试程序     |

## 支持的 SQL

```sql
-- 数据库操作
CREATE DATABASE mydb;
USE mydb;
DROP DATABASE mydb;

-- 表操作
CREATE TABLE users (id int primary, name string, age int);
DROP TABLE users;

-- 增删改查
INSERT INTO users VALUES (1, 'Alice', 25);
SELECT * FROM users WHERE age > 20;
SELECT id, name FROM users;
UPDATE users SET age = 26 WHERE id = 1;
DELETE FROM users WHERE id = 1;
```

### 数据类型

`int` `double` `string` `bool`

### WHERE 运算符

`=` `!=` `<>` `<` `<=` `>` `>=`

## 架构

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

## 数据存储

```
data/
└── mydb/                  # 每个数据库一个目录
    ├── users.dat          # 定长记录：261 字节/条
    ├── users.idx          # B+ 树索引：4KB/页
    └── users.meta         # 表结构元数据
```

## 通信协议

4 字节大端序长度头 + JSON 正文。

请求示例：

```json
{"command": "sql", "sql": "select * from users", "db": "mydb"}
```

响应示例：

```json
{"status": "ok", "result": {"columns": ["id", "name"], "rows": [[1, "Alice"]]}}
```

## 项目结构

```
Bind/
├── CMakeLists.txt
├── build.sh                    # 编译脚本
├── bind.sh                     # 一键启动
├── test.sh                     # 自动化测试
├── test_samples.sql            # SQL 集成测试用例
├── include/
│   ├── common/                 # ArrayList, LinkedList, String, Json, ResultSet
│   ├── storage/                # BPlusTree, FileEngine, Table, Database
│   ├── parser/                 # SQLParser
│   ├── executor/               # Executor
│   ├── network/                # TcpSocket
│   ├── server/                 # Server
│   └── client/                 # Client
├── src/                        # 实现文件
└── tests/
    └── test_main.cpp           # C++ 单元测试
```

## License

[MIT](LICENSE) © kabalts
