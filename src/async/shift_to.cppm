module;
#include <coroutine>
export module xin.async.shift_to;

import std;

import xin.async.awaiter;
import xin.async.io_context;


export namespace xin::async {

/// @brief 将当前 coroutine 切换到目标 IOContext 的 awaiter。
class ShiftToAwaiter : public Awaiter {
private:
    IOContext& target_;

public:
    /// @brief 构造一个目标上下文切换 awaiter。
    /// @param[in] context 目标 IOContext。
    explicit ShiftToAwaiter(IOContext& context) noexcept
      : target_{ context }
    {}

    /// @brief 若已在目标线程则无需挂起。
    /// @return true 表示可立即继续执行。
    constexpr auto await_ready() const noexcept -> bool
    {
        // 如果已经在目标线程上，无需切换，直接继续执行。
        return target_.is_owner_thread();
    }

    /// @brief 挂起当前 coroutine 并投递到目标 IOContext。
    /// @tparam Promise 当前 coroutine 的 promise 类型。
    /// @param[in] handle 当前 coroutine 句柄。
    /// @param[in] context 当前所在 IOContext（保留接口一致性）。
    /// @return `noop_coroutine`，等待目标线程恢复。
    template<typename Promise>
    auto await_suspend(std::coroutine_handle<Promise> handle, IOContext& context) noexcept
        -> std::coroutine_handle<>
    {
        this->handle = handle;

        if constexpr (requires { handle.promise().context; })
            handle.promise().context = &target_;

        // 会被目标线程唤醒
        target_.post(this);
        return std::noop_coroutine();
    }

    /// @brief 切换完成后返回成功。
    /// @return 始终返回 `std::expected<void, std::error_code>{}`。
    auto await_resume() const noexcept -> std::expected<void, std::error_code>
    {
        return {};
    }
};

/// @brief 构造 `shift_to` awaiter。
/// @param[in] context 目标 IOContext。
/// @return 对应的 `ShiftToAwaiter`。
auto shift_to(IOContext& context) -> ShiftToAwaiter
{
    return ShiftToAwaiter{ context };
}

} // namespace xin::async