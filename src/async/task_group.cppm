export module xin.async.task_group;

import std;

import xin.async.awaitable;
import xin.async.io_context;
import xin.async.post;
import xin.async.task;
import xin.async.this_coroutine;


namespace detail {

template<typename Awaitable>
auto spawn_task(Awaitable awaitable, std::shared_ptr<struct SpawnedState> state)
    -> xin::async::Task<>;

/// @brief TaskGroup 共享状态。
///
/// `pending_` 采用哨兵计数：初始值为 1，`join()` 时释放该哨兵。
struct SpawnedState {
    /// @brief 构造共享状态。
    /// @param[in] context 绑定的 IOContext。
    explicit SpawnedState(xin::async::IOContext& context)
      : context_{ &context }
    {}

    xin::async::IOContext* context_;
    std::stop_source stop_source_;
    std::stop_source drained_;
    std::atomic<std::size_t> pending_{ 1 }; // 1 sentinel: released by join()
};

/// @brief 包装已 spawn 的任务，结束时更新 `pending_` 并在 drain 条件满足时触发 stop。
template<typename Awaitable>
auto spawn_task(Awaitable awaitable, std::shared_ptr<SpawnedState> state) -> xin::async::Task<>
{
    co_await std::move(awaitable);

    if (state->pending_.fetch_sub(1, std::memory_order_acq_rel) == 1)
        state->drained_.request_stop();
}

} // namespace detail

export namespace xin::async {

/// @brief 等待 `stop_token` 被请求并恢复协程的 awaiter。
///
/// stop 回调触发时会把恢复动作投递到指定 `IOContext`，避免在回调线程直接 resume。
class StopRequestedAwaiter {
    using callback_type = std::stop_callback<std::function<void()>>;

    /// @brief stop 回调共享状态。
    struct State {
        std::atomic<bool> alive{ true };
        std::coroutine_handle<> handle;
    };

    IOContext* context_;
    std::stop_token token_;
    std::shared_ptr<State> state_{ std::make_shared<State>() };
    std::optional<callback_type> callback_;

public:
    /// @brief 绑定当前线程 context 的构造函数。
    /// @param[in] token 待监听的 stop_token。
    explicit StopRequestedAwaiter(std::stop_token token)
      : StopRequestedAwaiter(this_coroutine::context(), std::move(token))
    {}

    /// @brief 构造 awaiter。
    /// @param[in] context 恢复协程时使用的 IOContext。
    /// @param[in] token 待监听的 stop_token。
    StopRequestedAwaiter(IOContext& context, std::stop_token token)
      : context_(&context)
      , token_(std::move(token))
    {}

    /// @brief 析构时关闭 alive 标记并注销 stop 回调。
    ~StopRequestedAwaiter()
    {
        state_->alive.store(false, std::memory_order_release);
        callback_.reset();
    }

    [[nodiscard]]
    constexpr auto await_ready() const noexcept -> bool
    {
        return token_.stop_requested();
    }

    /// @brief 注册 stop 回调并挂起协程。
    auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool
    {
        state_->handle = handle;
        auto state = state_;
        auto* context = context_;

        callback_.emplace(token_, [state, context] {
            post(*context, [state] {
                if (state->alive.load(std::memory_order_acquire)) {
                    auto h = std::exchange(state->handle, std::coroutine_handle<>{});
                    if (h)
                        h.resume();
                }
            });
        });
        return true;
    }

    /// @brief 取消回调订阅并清理句柄。
    void await_resume() noexcept
    {
        callback_.reset();
        state_->handle = {};
    }
};

/// @brief 管理一组可协作停止的异步任务。
///
/// - `spawn()` 增加待完成计数并启动任务；
/// - `join()` 等待全部任务收敛；
/// - `request_stop()` 广播 stop_token。
class TaskGroup {
public:
    /// @brief 构造任务组。
    /// @param[in] context 任务组绑定的执行上下文。
    explicit TaskGroup(IOContext& context = this_coroutine::context())
      : state_{ std::make_shared<detail::SpawnedState>(context) }
    {}

    TaskGroup(const TaskGroup&) = delete;
    auto operator=(const TaskGroup&) -> TaskGroup& = delete;

    TaskGroup(TaskGroup&&) noexcept = default;
    auto operator=(TaskGroup&&) noexcept -> TaskGroup& = default;

    ~TaskGroup()
    {
        if (state_) {
            closed_ = true;
            request_stop();
        }
    }

    /// @brief 启动一个任务并纳入组内跟踪。
    /// @param[in] awaitable 待启动任务。
    template<awaitable Awaitable>
        requires std::movable<std::remove_cvref_t<Awaitable>>
    void spawn(Awaitable awaitable)
    {
        if (closed_)
            throw std::logic_error{ "Cannot spawn after join on async::Scope" };

        state_->pending_.fetch_add(1, std::memory_order_relaxed);
        co_spawn(detail::spawn_task(std::move(awaitable), state_), *state_->context_);
    }

    /// @brief 请求停止全部组内任务。
    void request_stop() noexcept
    {
        state_->stop_source_.request_stop();
    }

    [[nodiscard]]
    /// @brief 获取组级 stop_token。
    auto stop_token() const noexcept -> std::stop_token
    {
        return state_->stop_source_.get_token();
    }

    /// @brief 等待组内任务全部完成。
    ///
    /// 若释放哨兵后计数直接归零，立即返回；否则等待 drained stop 信号。
    auto join() -> Task<>
    {
        closed_ = true;

        // Release the sentinel count. If we transition 1 → 0 all spawned tasks
        // already finished (or none were spawned), so we are done immediately.
        if (state_->pending_.fetch_sub(1, std::memory_order_acq_rel) == 1)
            co_return;

        co_await StopRequestedAwaiter(state_->drained_.get_token());
    }

    /// @brief 获取任务组绑定的 context。
    auto context() const noexcept -> IOContext&
    {
        return *state_->context_;
    }

private:
    std::shared_ptr<detail::SpawnedState> state_;
    bool closed_{ false };
};

/// @brief 创建一个 `TaskGroup`。
/// @param[in] context 绑定的执行上下文。
/// @return `TaskGroup` 实例。
auto task_group(IOContext& context = this_coroutine::context()) -> TaskGroup
{
    return TaskGroup{ context };
}

} // namespace xin::async