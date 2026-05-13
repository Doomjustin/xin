[![Build & Test](https://github.com/Doomjustin/xin/actions/workflows/cmake.yml/badge.svg?branch=main)](https://github.com/Doomjustin/xin/actions/workflows/cmake.yml)

# xin

一个基于现代 C++23 的实验性异步运行时项目，使用 LLVM/libc++ modules、CMake、vcpkg 与 GitHub Actions 构建与验证。

## 项目状态

- 开发阶段：实验性（API 可能调整）
- 代码状态：可构建，可运行 Demo，包含单元测试
- CI：默认展示 `main` 分支状态

## 特性

- C++23 modules（包含 `import std` 试验链路）
- 基于 `io_uring` 的异步执行基础设施
- `Task`/`co_spawn`/`sleep_for` 等 coroutine 组合能力
- 可取消语义（`stop_token` 传播）
- CMake + vcpkg manifest + GitHub Actions

## 环境要求

- Linux
- LLVM/Clang（支持 C++23 modules）
- CMake 4.3+
- Ninja
- vcpkg（Manifest 模式）
- liburing（由 vcpkg 依赖链提供）

## 构建

仓库内置本地 preset：`linux-llvm-cxx23`。

```bash
cmake --preset linux-llvm-cxx23
cmake --build build
```

如果你的本机路径与工具链不同，请按需调整 [CMakePresets.json](CMakePresets.json)。

## 运行 Demo

```bash
cmake --build build --target xin.example.demo
./build/examples/xin.example.demo
```

当前 Demo 源码见 [examples/demo.cpp](examples/demo.cpp)。

## 最小 Demo

下面示例展示 `Task`、`co_spawn`、`sleep_for` 与 `stop_source` 的组合用法：

```cpp
import std;
import xin;

using namespace std::chrono_literals;
using namespace xin;

auto demo() -> async::Task<>
{
	log::info("Hello, async world!");
	co_await async::sleep_for(10s);
	log::info("Goodbye, async world!");
}

auto stop_then(std::stop_source stop_source) -> async::Task<>
{
	co_await async::co_spawn(demo());

	log::info("This task will be stopped in 3 seconds.");
	co_await async::sleep_for(3s);
	stop_source.request_stop();
	log::info("Stop requested.");
}

int main()
{
	async::IOContext context;

	std::stop_source stop_source;
	auto token = stop_source.get_token();

	async::co_spawn(context, token, demo());
	async::co_spawn(context, token, stop_then(stop_source));

	context.run();
	return 0;
}
```

## License

MIT，详见 [LICENSE](LICENSE)。
