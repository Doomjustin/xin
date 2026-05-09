[![Build & Test](https://github.com/Doomjustin/xin/actions/workflows/cmake.yml/badge.svg?branch=main)](https://github.com/Doomjustin/xin/actions/workflows/cmake.yml)

# xin

现代 C++23、LLVM/libc++ modules、vcpkg 与 GitHub Actions 驱动的实验性工程。

`xin` 当前聚焦于打通一条可持续演进的 modern C++ 工程链路，重点覆盖：

- CMake 4.3 + Ninja 构建
- vcpkg manifest 依赖管理
- GitHub Actions 持续集成
- 针对 `import std` / libc++ modules 的 CI 配置
- `clang-format + import` 自动整理脚本

当前代码仍在逐步补充中，但构建、CI 与开发工具链已经可以独立使用；目前工程基础设施、CI workflow 与本地 preset 已就绪，`src` 目录仍在逐步补充实际实现。

## Build

### Quick start

仓库当前提供的本地 preset：

- `linux-llvm-cxx23`

具体路径与缓存变量定义见 [CMakePresets.json](CMakePresets.json)。如果你的本机环境不同，需要先调整 preset。

本地构建：

```bash
cmake --preset linux-llvm-cxx23
cmake --build build
```

## CI

GitHub Actions workflow 位于 [./.github/workflows/cmake.yml](.github/workflows/cmake.yml)，当前会在以下事件触发：

- push 到 `main`
- push 到 `develop`
- pull request 到 `main`
- pull request 到 `develop`

README 顶部 badge 仅显示 `main` 分支状态。

## License

MIT. See [LICENSE](LICENSE).
