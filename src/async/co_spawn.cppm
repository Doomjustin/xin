export module xin.async.co_spawn;

import std;

import xin.async.awaitable;
import xin.async.io_context;
import xin.async.shift_to;
import xin.async.this_coroutine;


export namespace xin::async {

struct DetachedTask;

/// @brief fire-and-forget 任务句柄类型。
///
/// `DetachedTask` 本身不持有 coroutine handle，仅作为 `co_spawn` 的返回类型。
/// 生命周期由 promise 在协程内部自管理。
///
/// @note 该类型不提供结果获取接口；若需要结果传递与聚合，请使用 `Task`。
struct DetachedTask {
    /// @brief `DetachedTask` 的 promise 类型。
    ///
    /// 该 promise 会在构造时为 `IOContext` 增加 work 计数，析构时减少 work 计数，
    /// 以确保 detached 协程执行期间 `IOContext::run()` 不会提前退出。
    struct promise_type {
        IOContext* context = nullptr;

        /// @brief 构造 promise 并绑定运行上下文。
        /// @tparam Awaitable 被 `co_spawn` 启动的 awaitable 类型。
        /// @param[in] awaitable 启动参数占位，用于与 coroutine 参数签名匹配。
        /// @param[in] ctx 执行上下文，默认使用 `this_coroutine::context()`。
        /// @note 构造时调用 `add_work()`，保证 detached 协程未结束前 `IOContext` 不会因 work
        /// 计数归零而退出。
        template<typename Awaitable>
        promise_type(Awaitable&& awaitable)
          : context{ &this_coroutine::context() }
        {
            context->add_work();
        }

        template<typename Awaitable>
        promise_type(IOContext& context, Awaitable&& awaitable)
          : context{ &context }
        {
            context.add_work();
        }

        ~promise_type()
        {
            context->drop_work();
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

        /// @brief detached 协程出现未处理异常时直接终止进程。
        void unhandled_exception() noexcept
        {
            std::terminate();
        }
    };
};

/// @brief 在指定 `IOContext` 上启动一个 detached awaitable。
///
/// 启动流程分两步：
/// 1) `co_await shift_to(context)` 切换到目标上下文线程；
/// 2) 在该上下文执行 `awaitable`。
///
/// 该函数采用 fire-and-forget 语义，不向调用方返回执行结果。
///
/// 示例：
/// ```cpp
/// auto worker = []() -> xin::async::Task<void> {
///     co_return;
/// };
///
/// xin::async::IOContext ctx;
/// xin::async::co_spawn(ctx, worker());
/// ctx.run();
/// ```
///
/// @tparam Awaitable 满足 `awaitable` concept 的类型。
/// @param[in] context 目标执行上下文。
/// @param[in] awaitable 待执行的 awaitable 对象。
/// @return `DetachedTask` 占位对象。
template<awaitable Awaitable>
    requires std::movable<std::remove_cvref_t<Awaitable>>
auto co_spawn(IOContext& context, Awaitable awaitable) -> DetachedTask
{
    co_await shift_to(context);
    co_await std::move(awaitable);
}

/// @brief 在当前绑定上下文上启动一个 detached awaitable。
///
/// 该重载会通过 `this_coroutine::context()` 获取当前绑定的 `IOContext`，
/// 再转调 `co_spawn(context, awaitable)`。
///
/// 本函数采用 fire-and-forget 语义，不返回结果。
///
/// 示例：
/// ```cpp
/// auto worker = []() -> xin::async::Task<void> {
///     co_return;
/// };
///
/// // 运行线程中已绑定 context 时可直接调用：
/// xin::async::co_spawn(worker());
/// ```
///
/// @tparam Awaitable 满足 `awaitable` concept 的类型。
/// @param[in] awaitable 待执行的 awaitable 对象。
/// @return `DetachedTask` 占位对象。
template<awaitable Awaitable>
    requires std::movable<std::remove_cvref_t<Awaitable>>
auto co_spawn(Awaitable awaitable) -> DetachedTask
{
    return co_spawn(this_coroutine::context(), std::move(awaitable));
}

} // namespace xin::async