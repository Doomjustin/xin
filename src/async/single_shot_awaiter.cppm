module;

#include <liburing.h>

export module xin.async.single_shot_awaiter;

import std;

import xin.async.awaiter;
import xin.async.io_context;
import xin.utility;


export namespace xin::async {

/// @brief 基于 io_uring 的 single-shot Awaiter 基类。
///
/// Derived 需要提供以下接口：
/// - void prepare(::io_uring_sqe* sqe)：用于准备 SQE。
/// - 当 Resume 非 void 时，提供 result() 用于返回业务结果。
///
/// 该类在 `await_suspend` 时只提交一次 SQE，并将自身作为 user data
/// 关联到 CQE。若当前无法获取 SQE，则以 EAGAIN 结束本次挂起流程。
///
/// @tparam Derived 具体 Awaiter 类型，使用 CRTP。
/// @tparam Resume await_resume 的返回类型。
template<typename Derived, typename Resume = void>
class SingleShotAwaiter : public Awaiter {
public:
    constexpr auto await_ready() const noexcept -> bool
    {
        return false;
    }

    /// @brief 挂起协程并尝试提交一次 SQE。
    /// @param[in] handle 当前协程句柄。
    /// @return true 表示已成功提交并保持挂起；false 表示提交失败并立即恢复。
    auto await_suspend(std::coroutine_handle<> handle, IOContext& context) noexcept
        -> std::coroutine_handle<>
    {
        this->handle = handle;

        if (auto* sqe = context.sqe()) {
            static_cast<Derived*>(this)->prepare(sqe);
            context.track(sqe, this);
            return std::noop_coroutine();
        }

        result = -EAGAIN;
        return handle;
    }

    /// @brief 在 completion 后恢复协程并返回结果。
    /// @return 当 Resume 为 void 时无返回值；否则返回 Derived::result() 的结果。
    /// 失败时返回 unexpected_system_error。
    auto await_resume() noexcept -> std::expected<Resume, std::error_code>
    {
        if (result < 0)
            return unexpected_system_error(-result);

        if constexpr (std::is_void_v<Resume>)
            return {};
        else
            return static_cast<Derived*>(this)->result();
    }
};

} // namespace xin::async