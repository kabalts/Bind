[English](README_EN.md) | 中文

# Bind — Bind Integrates Nested Data

一个用 C++23 从零构建的轻量级关系型数据库，包含自定义 B+ 树存储引擎、递归下降 SQL 解析器和 TCP 网络协议。

## 特性

- **自制存储引擎**：B+ 树索引 + 定长记录文件，4KB 页大小，支持磁盘持久化
- **完整 SQL 支持**：DDL (CREATE/DROP/USE) + DML (SELECT/INSERT/UPDATE/DELETE)，递归下降解析器
- **主键唯一约束**：基于 B+ 树索引进阶保证
- **客户端-服务端架构**：长度前缀帧协议 (TCP)，JSON 格式通信
- **无第三方依赖**：仅使用 C++ 标准库 + Windows API，自实现 ArrayList、LinkedList、String、Json 等容器

## 快速开始

从 [Releases](https://github.com/kabalts/Bind/releases) 下载编译好的可执行文件，或自行编译：

```powershell
# 一键启动（服务端 + 客户端）
.\bind.ps1
```

### 环境要求

| 组件     | 版本                     |
| -------- | ------------------------ |
| 操作系统 | Windows 11            |
| 编译器   | MSVC (Visual Studio 2026)|
| 标准     | C++23                    |
| 构建工具 | CMake 3.15+              |

## 构建

### 安装依赖

安装 [Visual Studio 2026](https://visualstudio.microsoft.com/)，勾选 **"使用 C++ 的桌面开发"** 工作负载，以及 [CMake](https://cmake.org/download/)。

### 在 Visual Studio 中构建

直接用 VS 打开项目文件夹，CMakeLists.txt 会被自动识别，然后 **生成 → 全部生成**。

### 命令行构建

```powershell
cmake -B out/build/x64-Debug
cmake --build out/build/x64-Debug
```

### 编译产物

| 文件                      | 说明         |
| ------------------------- | ------------ |
| `out/build/x64-Debug/bin/bind_server.exe` | 数据库服务端 |
| `out/build/x64-Debug/bin/bind_client.exe` | 命令行客户端 |

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
├── bind.ps1                    # 一键启动（服务端 + 客户端）
├── include/
│   ├── common/                 # ArrayList, LinkedList, String, Json, ResultSet
│   ├── storage/                # BPlusTree, FileEngine, Table, Database
│   ├── parser/                 # SQLParser
│   ├── executor/               # Executor
│   ├── network/                # TcpSocket
│   ├── server/                 # Server
│   └── client/                 # Client
├── src/                        # 实现文件
```

## License

[MIT](LICENSE) © kabalts
