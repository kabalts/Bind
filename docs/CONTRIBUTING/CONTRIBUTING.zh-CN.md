# 参与贡献

感谢你对 Bind 项目的关注！

## 环境准备

```powershell
# 安装 Visual Studio 2026，勾选"使用 C++ 的桌面开发"工作负载
# 安装 CMake：https://cmake.org/download/

# 克隆仓库

# GitHub
git clone https://github.com/kabalts/Bind.git

# Gitee
git clone https://gitee.com/kabalts/Bind.git

cd Bind

# 构建
cmake -B out/build/x64-Debug
cmake --build out/build/x64-Debug
```

## 代码风格

- C++23 标准
- 不使用 STL 容器，项目自带 ArrayList、LinkedList、String、Json 等实现
- 头文件使用 `#pragma once`
- 类名 PascalCase (`BPlusTree`)，方法名 snake_case (`find_database`)
- 成员变量前缀 `m_` (`m_current_db`)
- 不加注释，保持代码自解释

## 提交 PR

1. Fork 本仓库
2. 创建特性分支：`git checkout -b feature/your-feature`
3. 编写代码后提交
4. 提交 commit：`git commit -m "feat: add your feature"`
5. 推送并创建 Pull Request

## 问题反馈

请通过 [GitHub Issues](https://github.com/kabalts/Bind/issues) 或 [Gitee Issues](https://gitee.com/kabalts/Bind/issues) 提交 Bug 报告或功能建议。
