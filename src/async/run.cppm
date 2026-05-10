export module xin.async.run;

import std;

import xin.async.io_context;
import xin.async.single_shot_operation;
import xin.async.task;
import xin.async.this_coroutine;
import xin.utility;


namespace detail {

using IOContext = xin::async::IOContext;

inline std::vector<IOContext*> active_contexts;
inline std::mutex contexts_mutex;

/// @brief 记录一个活跃 context，供 `stop()` 广播使用。
void push(IOContext& context)
{
    std::scoped_lock locker{ contexts_mutex };
    active_contexts.push_back(&context);
}

/// @brief 从活跃列表移除 context。
void erase(IOContext& context)
{
    std::scoped_lock locker{ contexts_mutex };
    std::erase_if(active_contexts, [&context](IOContext* ctx) { return ctx == &context; });
}

/// @brief 作用域化登记活跃 context 的 RAII 守卫。
struct ContextGuard {
    /// @brief 构造时登记 context。
    explicit ContextGuard(IOContext& ctx)
      : ctx_{ ctx }
    {
        push(ctx_);
    }

    ContextGuard(const ContextGuard&) = delete;
    auto operator=(const ContextGuard&) -> ContextGuard& = delete;

    /// @brief 析构时移除 context。
    ~ContextGuard()
    {
        erase(ctx_);
    }

private:
    IOContext& ctx_;
};

} // namespace detail

export namespace xin::async {

/// @brief 在当前线程 context 上运行一个 awaitable 入口。
///
/// 该函数会：
/// 1) 获取当前线程 context；
/// 2) 通过 `co_spawn` 启动任务；
/// 3) 调用 `context.run()` 驱动事件循环。
///
/// @tparam Awaiter 可调用对象类型，调用后应返回可等待对象。
/// @tparam Args 传给 awaiter 的参数类型。
/// @param[in] awaiter 任务入口可调用对象。
/// @param[in] args 传给入口的参数。
template<typename Awaiter, typename... Args>
    requires std::copy_constructible<Awaiter> && (std::copy_constructible<Args> && ...)
void run(Awaiter&& awaiter, Args&&... args)
{
    auto& context = this_coroutine::context();

    detail::ContextGuard guard{ context };
    co_spawn(context, std::invoke(awaiter, args...));
    context.run();
}

/// @brief 在多线程上运行同一 awaitable 入口。
///
/// 会创建 `thread_count` 个运行线程（包含当前线程），每个线程各自
/// 获取自己的 thread-local context 并独立执行 `run()`。
///
/// @tparam Awaiter 可调用对象类型，调用后应返回可等待对象。
/// @tparam Args 传给 awaiter 的参数类型。
/// @param[in] thread_count 运行线程数量。
/// @param[in] awaiter 任务入口可调用对象。
/// @param[in] args 传给入口的参数。
template<typename Awaiter, typename... Args>
    requires std::copy_constructible<Awaiter> && (std::copy_constructible<Args> && ...)
void run(std::integral auto thread_count, Awaiter&& awaiter, Args&&... args)
{
    std::vector<std::jthread> threads;

    for (int i = 1; i < thread_count; ++i) {
        threads.emplace_back([awaiter, args...]() mutable -> void {
            auto& context = this_coroutine::context();

            detail::ContextGuard guard{ context };
            co_spawn(context, std::invoke(awaiter, args...));
            context.run();
        });
    }

    run(std::forward<Awaiter>(awaiter), std::forward<Args>(args)...);
}

/// @brief 运行一个接收 `stop_token` 的入口。
/// @param[in] source 停止源，用于向任务提供 `stop_token`。
/// @param[in] awaiter 任务入口。
template<typename Awaiter>
    requires std::copy_constructible<Awaiter>
void run(std::stop_source& source, Awaiter&& awaiter)
{
    auto coro = [token = source.get_token(),
                 f = std::forward<Awaiter>(awaiter)]() mutable -> Task<> {
        co_await std::invoke(f, token);
    };

    run(coro);
}

/// @brief 多线程运行一个接收 `stop_token` 的入口。
/// @param[in] thread_count 运行线程数量。
/// @param[in] source 停止源，用于向任务提供 `stop_token`。
/// @param[in] awaiter 任务入口。
template<typename Awaiter>
    requires std::copy_constructible<Awaiter>
void run(std::integral auto thread_count, std::stop_source& source, Awaiter&& awaiter)
{
    auto coro = [token = source.get_token(),
                 f = std::forward<Awaiter>(awaiter)]() mutable -> Task<> {
        co_await std::invoke(f, token);
    };

    run(thread_count, coro);
}

/// @brief 请求停止当前由 `run(...)` 管理的全部活跃 context。
void stop()
{
    std::scoped_lock lock{ detail::contexts_mutex };
    std::ranges::for_each(detail::active_contexts,
                          [](IOContext* context) -> void { context->stop(); });

    // 每个context在被stop之后都会从active_contexts里把自己删除，所以这里不需要clear
}

} // namespace xin::async