export module xin.async.shift_to;

import std;

import xin.async.io_context;
import xin.async.operation;


export namespace xin::async {

/// @brief 用于恢复目标协程的调度操作。
///
/// 该对象由 `ShiftToAwaiter` 在 `await_suspend` 中动态创建，
/// 完成后会自销毁并恢复挂起协程。
class ShiftToOperation final : public Operation {
public:
    /// @brief 构造恢复操作。
    /// @param[in] handle 需要在目标 context 线程恢复的协程句柄。
    explicit ShiftToOperation(std::coroutine_handle<> handle) noexcept
      : handle_{ handle }
    {}

    /// @brief 调度完成回调。
    /// @param[in] res 调度结果（当前实现未使用）。
    /// @param[in] flags CQE flags（当前实现未使用）。
    void complete(int res, unsigned flags) noexcept override
    {
        auto handle = std::exchange(handle_, {});
        delete this;
        if (handle)
            handle.resume();
    }

private:
    std::coroutine_handle<> handle_{ nullptr };
};

/// @brief 将当前协程切换到指定 `IOContext` 的 awaiter。
class ShiftToAwaiter {
public:
    /// @brief 绑定目标 context。
    /// @param[in] context 目标执行上下文。
    explicit ShiftToAwaiter(IOContext& context) noexcept
      : context_{ &context }
    {}

    [[nodiscard]]
    constexpr auto await_ready() const noexcept -> bool
    {
        return false;
    }

    /// @brief 挂起当前协程并投递恢复操作。
    ///
    /// 在 owner thread 上走 `submit`，否则走跨线程 `post`。
    /// @param[in] handle 当前挂起协程句柄。
    /// @return 始终返回 `true`，表示挂起并等待后续恢复。
    [[nodiscard]]
    auto await_suspend(std::coroutine_handle<> handle) const noexcept -> bool
    {
        auto* op = new ShiftToOperation{ handle };
        if (context_->is_owner_thread())
            context_->submit(op);
        else
            context_->post(op);
        return true;
    }

    void await_resume() const noexcept {}

private:
    IOContext* context_;
};

/// @brief 生成一个切换到目标 context 的 awaiter。
/// @param[in] context 目标执行上下文。
/// @return `ShiftToAwaiter`。
auto shift_to(IOContext& context) noexcept -> ShiftToAwaiter
{
    return ShiftToAwaiter{ context };
}

} // namespace xin::async
