module;

#include <liburing.h>

export module xin.async.timeout;

import std;

import xin.async.io_context;
import xin.async.operation;
import xin.async.single_shot_operation;
import xin.async.sleep;
import xin.async.this_coroutine;
import xin.async.when_any;
import xin.utility;


export namespace xin::async {

/// @brief 为 single-shot operation 提供 linked-timeout 语义。
///
/// 该实现会同时提交两条 SQE：
/// 1) 原始 IO 请求（带 `IOSQE_IO_LINK`）；
/// 2) 对应 `link_timeout` 请求。
///
/// 当超时先完成时，`await_resume()` 返回 `timed_out`。
template<single_shot_operation InnerOperation>
class TimeoutAwaiter : public Operation {
public:
    using resume_type = typename InnerOperation::resume_type;

    /// @brief 构造超时 awaiter。
    /// @param[in] operation 被包装的 single-shot operation。
    /// @param[in] timeout 超时时长。
    template<chrono_duration Duration>
    TimeoutAwaiter(InnerOperation&& operation, Duration timeout)
      : inner_operation_{ std::forward<InnerOperation>(operation) }
    {
        using namespace std::chrono;

        auto ns = duration_cast<nanoseconds>(timeout).count();
        timeout_.tv_sec = ns / 1'000'000'000;
        timeout_.tv_nsec = ns % 1'000'000'000;
    }

    [[nodiscard]]
    constexpr auto await_ready() const noexcept
    {
        return false;
    }

    /// @brief 提交原始 IO 与 link-timeout。
    /// @return 成功提交返回 `true`；SQE 不足时返回 `false` 并以 `EAGAIN` 失败。
    auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool
    {
        handle_ = handle;

        auto* io_sqe = context().sqe();
        auto* timeout_sqe = context().sqe();
        if (!io_sqe || !timeout_sqe) {
            sanitize_sqe(io_sqe);
            sanitize_sqe(timeout_sqe);

            result_ = -EAGAIN;
            return false;
        }

        inner_operation_.prepare(io_sqe);
        io_sqe->flags |= IOSQE_IO_LINK;
        ::io_uring_sqe_set_data(io_sqe, this);

        ::io_uring_prep_link_timeout(timeout_sqe, &timeout_, 0);
        ::io_uring_sqe_set_data(timeout_sqe, this);

        context().track(this);
        return true;
    }

    /// @brief 获取完成结果。
    /// @return 超时时返回 `timed_out`；否则转发 inner operation 的结果。
    auto await_resume() noexcept -> std::expected<resume_type, std::error_code>
    {
        if (is_timed_out_)
            return unexpected_system_error(std::errc::timed_out);

        if (result_ < 0)
            return unexpected_system_error(-result_);

        if constexpr (!std::is_void_v<resume_type>)
            inner_operation_.set_result(result_, 0);

        return inner_operation_.await_resume();
    }

    /// @brief 聚合两条 CQE 的结果状态。
    void set_result(int result, unsigned flags) noexcept
    {
        if (result == -ETIME)
            is_timed_out_ = true;
        else if (result != -ECANCELED)
            result_ = result;
    }

    /// @brief 处理完成回调；等待两条 CQE 都到达后再恢复协程。
    void complete(int result, unsigned flags) noexcept override
    {
        set_result(result, flags);

        if (--pending_cqes_ == 0) {
            context().untrack(this);
            this->resume(handle_, result_, flags);
        }
    }

    /// @brief 转发取消请求到 context。
    void cancel() noexcept
    {
        context().cancel(this);
    }

    /// @brief 获取 inner operation 绑定的 context。
    auto context() noexcept -> IOContext&
    {
        return inner_operation_.context();
    }

private:
    /// @brief 将已占用但未使用的 SQE 置为 nop，避免提交脏条目。
    static void sanitize_sqe(::io_uring_sqe* sqe) noexcept
    {
        if (!sqe)
            return;

        ::io_uring_prep_nop(sqe);
        ::io_uring_sqe_set_data(sqe, nullptr);
    }

    InnerOperation inner_operation_;
    __kernel_timespec timeout_;

    std::coroutine_handle<> handle_;
    int pending_cqes_{ 2 };
    bool is_timed_out_{ false };
    int result_{ -ECANCELED };
};

/// @brief 为一般 cancelable operation 提供 timeout 语义包装。
///
/// 该实现基于 `WhenAnyAwaiter<Op, TimerAwaier>`：
/// timer 胜出则返回 `timed_out`，否则返回原操作结果。
template<cancelable_operation Op>
class TimeoutWrapper : public Operation {
public:
    using resume_type = typename Op::resume_type;

    /// @brief 构造包装器。
    /// @param[in] op 被包装的可取消操作。
    /// @param[in] sleep 对应超时 timer。
    TimeoutWrapper(Op&& op, TimerAwaier sleep)
      : inner_{ std::move(op), std::move(sleep) }
    {}

    [[nodiscard]]
    constexpr auto await_ready() const noexcept -> bool
    {
        return false;
    }

    /// @brief 挂起当前协程，并把本对象挂到 inner 的 parent 链上。
    auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool
    {
        handle_ = handle;
        inner_.parent = this;
        return inner_.await_suspend(handle);
    }

    /// @brief 根据 winner 解析结果。
    /// @return timer 胜出返回 `timed_out`；否则返回原操作结果。
    auto await_resume() -> std::expected<resume_type, std::error_code>
    {
        if (inner_.winner() == 1)
            return unexpected_system_error(std::errc::timed_out);

        if constexpr (std::is_void_v<resume_type>) {
            return inner_.await_resume();
        }
        else {
            auto result = inner_.await_resume();
            return std::get<0>(result);
        }
    }

    /// @brief 获取 inner awaiter 绑定的 context。
    auto context() noexcept -> decltype(auto)
    {
        return inner_.context();
    }

    /// @brief 收到 inner 完成后恢复上层协程。
    void complete(int result, std::uint32_t flags) noexcept override
    {
        this->resume(handle_, result, flags);
    }

    /// @brief 转发取消请求。
    void cancel() noexcept
    {
        inner_.cancel();
    }

private:
    WhenAnyAwaiter<Op, TimerAwaier> inner_;
    std::coroutine_handle<> handle_;
};

/// @brief 为 single-shot operation 添加 timeout。
/// @param[in] operation 被包装操作。
/// @param[in] dur 超时时长。
/// @return `TimeoutAwaiter`。
template<single_shot_operation Operation, chrono_duration Duration>
auto timeout(Operation&& operation, Duration dur)
{
    return TimeoutAwaiter<std::decay_t<Operation>>{ std::forward<Operation>(operation), dur };
}

/// @brief 为一般 cancelable operation 添加 timeout。
/// @param[in] operation 被包装操作。
/// @param[in] dur 超时时长。
/// @return `TimeoutWrapper`。
template<cancelable_operation Operation, chrono_duration Duration>
    requires(!single_shot_operation<Operation>)
auto timeout(Operation&& operation, Duration dur)
{
    auto& ctx = operation.context();
    return TimeoutWrapper<std::decay_t<Operation>>{ std::forward<Operation>(operation),
                                                    TimerAwaier{ ctx, dur } };
}

} // namespace xin::async