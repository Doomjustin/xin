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

int main(int argc, char* argv[])
{
    std::stop_source stop_source;
    auto token = stop_source.get_token();
    async::run(4, token, stop_then, stop_source);
    return 0;
}
```

## xin::async 模块对齐说明

本文档用于对齐 `src/async` 子模块的职责边界、执行语义与常见组合方式。

### 目标

- 提供基于 `io_uring` 的 coroutine 运行时基础设施。
- 统一 `Task`、`Awaiter`、`IOContext` 与 `co_spawn/run` 的语义约定。
- 为跨线程调度与 `stop_token` 取消传播提供可预期行为。

### 组件分层

- `awaiter.cppm`：最基础的调度节点与 continuation 转发。
- `io_context.cppm`：事件循环、SQE/CQE 驱动、跨线程投递与取消。
- `single_shot_awaiter.cppm` / `timer_awaiter.cppm`：基于 `io_uring` 的 awaiter 适配层。
- `task.cppm`：lazy 启动的可组合 `Task<T>`。
- `stoppable_promise.cppm`：`stop_token` 注入与取消传播包装。
- `shift_to.cppm`：切换 coroutine 执行归属 `IOContext`。
- `co_spawn.cppm`：fire-and-forget 启动入口。
- `run.cppm`：便捷运行入口（单线程/多线程重载）。
- `sleep.cppm`：时间相关 awaitable 便捷接口。
- `this_coro.cppm`：读取当前 coroutine 环境标签（`context`/`stop_token`）。
- `async.cppm`：聚合导出入口。

### 核心语义

- `Task<T>` 为 lazy 语义：构造后不会立即执行。
- `co_spawn` 为 detached 语义：异常不向调用方传播，内部终止处理。
- `shift_to` 在目标线程已匹配时不挂起，否则通过 `IOContext::post` 异步切换。
- `run` 系列重载创建并驱动 `IOContext`，直到 tracked work 清零。
- 当 `stop_token` 可用时，`StoppablePromise` 会为 `Awaiter` 派生类型安装取消回调。

### 设计约束

- 线程归属：`IOContext` 绑定事件循环线程，`shift_to` 仅负责将 coroutine 恢复投递到目标 `IOContext`。
- 任务生命周期：`co_spawn` 的 detached coroutine 由 promise 内部 `track/untrack` 参与 work 计数，不依赖外部句柄持有。
- awaiter 约束：需要接入取消传播的 awaitable 应继承 `Awaiter`，以便被 `StoppablePromise` 自动包装。
- 调度边界：跨线程恢复统一走 `IOContext::post`，同线程短路径可直接进入本地恢复队列。

### 取消语义

- `stop_token` 为协作式取消信号，不保证“立即停止”。
- 当 awaitable 为 `Awaiter` 派生类型时，`stop_callback` 会触发 `IOContext::cancel` 提交取消请求。
- await_resume 阶段若确认已取消，返回 `operation_canceled`，调用方应显式处理错误分支。

### 异常语义

- `Task<T>` 捕获 coroutine 内未处理异常，并在 `await_resume` 时重新抛出。
- detached 的 `co_spawn` 不向调用方传播异常，当前策略为 `std::terminate`。
- 对于业务可恢复错误，建议优先使用 `std::expected` 在协程边界传递。

### 使用建议

- 在应用入口优先使用 `run(...)` 驱动任务树，避免手动管理 `IOContext` 生命周期。※目前没有实现work guard，所以context会在任务完成后自动退出
- 需要线程切换时显式 `co_await shift_to(target_context)`，不要依赖隐式线程亲和。
- 对 `std::expected` 结果先判断再解引用，尤其在 timeout/cancel 分支避免未定义行为。
- 若组合多个 awaitable，建议统一错误模型（例如都返回 `std::expected<..., std::error_code>`）。

## License

MIT，详见 [LICENSE](LICENSE)。
