# 参与贡献

感谢你对 Bind 项目的关注！

我还尝试了 Bind for windows !

你能在 gitee 中查看 [Bind for windows](https://gitee.com/kabalts/Bind-for-windows.git)

或者在 github 中查看 `win` 分支

## 环境准备

```bash
# 安装依赖（Ubuntu 26.04）
sudo apt install g++ cmake make

# 克隆仓库

# GitHub
git clone https://github.com/kabalts/Bind.git

# Gitee
git clone https://gitee.com/kabalts/Bind.git

cd Bind

# 编译
bash build.sh

# 运行测试，确保通过
bash test.sh
```

## 代码风格

- C++23 标准，使用 `-std=c++23`
- 不使用 STL 容器，项目自带 ArrayList、LinkedList、String、Json 等实现
- 头文件使用 `#pragma once`
- 类名 PascalCase (`BPlusTree`)，方法名 snake_case (`find_database`)
- 成员变量前缀 `m_` (`m_current_db`)
- 不加注释，保持代码自解释

## 提交 PR

1. Fork 本仓库
2. 创建特性分支：`git checkout -b feature/your-feature`
3. 编写代码并确保 `bash test.sh` 全部通过
4. 提交 commit：`git commit -m "feat: add your feature"`
5. 推送并创建 Pull Request

## 问题反馈

请通过 [GitHub Issues](https://github.com/kabalts/Bind/issues) 或 [Gitee Issues](https://gitee.com/kabalts/Bind/issues) 提交 Bug 报告或功能建议。
