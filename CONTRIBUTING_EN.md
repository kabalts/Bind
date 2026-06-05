# Contributing

Thanks for your interest in Bind!

## Environment Setup

```powershell
# Install Visual Studio 2026 with "Desktop development with C++" workload
# Install CMake: https://cmake.org/download/

# Clone the repository

# GitHub
git clone https://github.com/kabalts/Bind.git

# Gitee
git clone https://gitee.com/kabalts/Bind.git

cd Bind

# Build
cmake -B out/build/x64-Debug
cmake --build out/build/x64-Debug
```

## Code Style

- C++23 standard
- No STL containers — use the project's built-in ArrayList, LinkedList, String, Json, etc.
- Header guards use `#pragma once`
- Class names: PascalCase (`BPlusTree`), method names: snake_case (`find_database`)
- Member variables prefixed with `m_` (`m_current_db`)
- No comments — code should be self-explanatory

## Submitting a PR

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/your-feature`
3. Write code and commit
4. Commit: `git commit -m "feat: add your feature"`
5. Push and open a Pull Request

## Reporting Issues

Please submit bug reports or feature requests via [GitHub Issues](https://github.com/kabalts/Bind/issues) or [Gitee Issues](https://gitee.com/kabalts/Bind/issues).
