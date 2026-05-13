export module xin.async.co_spawn;

import std;

import xin.async.io_context;
import xin.async.task;
import xin.async.this_coro;


export namespace xin::async {

/// @brief 用于 fire-and-forget 执行的 coroutine 返回类型。
struct DetachedTask {
    /// @brief DetachedTask 的 promise，实现 IOContext 跟踪与 stop_token 传播。
    struct promise_type {
        IOContext* context{ nullptr };
        std::stop_token stop_token;

        template<typename Awaitable>
        promise_type(IOContext& ctx, Awaitable&& awaitable)
          : context{ &ctx }
        {
            context->track();
        }

        template<typename Awaitable>
        promise_type(IOContext& ctx, std::stop_token token, Awaitable&& awaitable)
          : context{ &ctx }
          , stop_token{ std::move(token) }
        {
            context->track();
        }

        ~promise_type()
        {
            context->untrack();
        }

        auto get_return_object() noexcept
        {
            return DetachedTask{};
        }

        auto initial_suspend() noexcept
        {
            return std::suspend_never{};
        }

        auto final_suspend() noexcept
        {
            return std::suspend_never{};
        }

        void return_void() noexcept {}

        void unhandled_exception() noexcept
        {
            std::terminate();
        }
    };
};

/// @brief 在指定 IOContext 上启动一个 detached coroutine。
/// @param[in] context 任务绑定的 IOContext。
/// @param[in] awaitable 要执行的 awaitable 对象。
/// @return DetachedTask。
template<typename Awaitable>
    requires std::movable<std::remove_cvref_t<Awaitable>>
auto co_spawn(IOContext& context, Awaitable awaitable) -> DetachedTask
{
    co_await std::move(awaitable);
}

/// @brief 在指定 IOContext 上启动 detached coroutine，并注入外部 stop_token。
/// @param[in] context 任务绑定的 IOContext。
/// @param[in] stop_token 用于向任务传播停止请求的 stop_token。
/// @param[in] awaitable 要执行的 awaitable 对象。
/// @return DetachedTask。
template<typename Awaitable>
    requires std::movable<std::remove_cvref_t<Awaitable>>
auto co_spawn(IOContext& context, std::stop_token stop_token, Awaitable awaitable) -> DetachedTask
{
    co_await std::move(awaitable);
}

/// @brief 在当前 Task 环境中启动 detached coroutine（自动继承 context 与 stop_token）。
/// @param[in] awaitable 要执行的 awaitable 对象。
/// @return Task<>。
///
/// 该重载通过 `this_coro::context` 与 `this_coro::stop_token` 获取当前 coroutine 环境，
/// 因此应在拥有 Task promise 上下文的 coroutine 内使用。
template<typename Awaitable>
    requires std::movable<std::remove_cvref_t<Awaitable>>
auto co_spawn(Awaitable awaitable) -> Task<>
{
    auto* ctx = co_await this_coro::context;
    auto token = co_await this_coro::stop_token;

    co_spawn(*ctx, std::move(token), std::move(awaitable));
}

} // namespace xin::async