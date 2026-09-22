# Bind — 运行环境与流程

## 系统环境

- **操作系统**: Windows 11
- **编译器**: MSVC (Visual Studio 2026)
- **标准**: C++23
- **构建**: CMake 3.15+
- **外部依赖**:
  - C++ 标准库：`<cstdio>` `<cstring>` `<cstdlib>` `<cstdint>` `<cstddef>` `<cmath>` `<cassert>` `<cctype>` `<cerrno>` `<algorithm>` `<new>` `<type_traits>` `<utility>` `<memory>` `<iterator>` `<stdexcept>` `<initializer_list>` `<iostream>` `<ostream>` `<string>`
  - Windows API：`<winsock2.h>` `<ws2tcpip.h>` `<windows.h>`
  - 无第三方库

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

## 启停流程

```powershell
# 终端1：启动服务端
.\out\build\x64-Debug\bin\bind_server.exe -p 8888

# 终端2：启动客户端
.\out\build\x64-Debug\bin\bind_client.exe
```

- 服务端窗口保持运行，客户端窗口进入交互模式
- Ctrl+C / `exit` 退出客户端，关闭服务端窗口即可停止

## 项目结构

```
Bind/
├── CMakeLists.txt
├── include/
│   ├── common/            # ArrayList, LinkedList, String, Json, ResultSet
│   ├── storage/           # BPlusTree, FileEngine, Table, Database
│   ├── parser/            # SQLParser
│   ├── executor/          # Executor
│   ├── network/           # TcpSocket
│   ├── server/            # Server
│   └── client/            # Client
├── src/                   # 对应实现文件
│   ├── common/
│   ├── storage/
│   ├── parser/
│   ├── executor/
│   ├── network/
│   ├── server/
│   └── client/
│   ├── main_server.cpp
│   └── main_client.cpp
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
| 正常使用 | 8888 |
