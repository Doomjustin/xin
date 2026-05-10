module;

#include <liburing.h>

export module xin.async.loop_operation;

import std;

import xin.async.io_context;
import xin.async.operation;
import xin.utility;


export namespace xin::async {

/// @brief 面向“分段提交”的循环 IO 基类。
///
/// 派生类需实现 `arm()`，用于提交下一次 IO 请求。基类负责：
/// 1) 聚合已处理字节数；
/// 2) 推进 buffer 子区间；
/// 3) 在完成/错误/取消条件下恢复协程。
template<typename Derived, typename Span>
class LoopOperation : public Operation {
public:
    using resume_type = std::size_t;
    using context_type = IOContext;

    /// @brief 构造循环操作。
    /// @param[in] context 绑定的 IOContext。
    /// @param[in] buffer 待处理缓冲区视图。
    LoopOperation(context_type& context, Span buffer)
      : context_{ &context }
      , buffer_{ buffer }
    {}

    [[nodiscard]]
    constexpr auto await_ready() const noexcept -> bool
    {
        return false;
    }

    /// @brief 挂起当前协程并由派生类提交首个请求。
    /// @return `arm()` 成功时返回 `true`；失败时返回 `false`。
    auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool
    {
        handle_ = handle;
        return static_cast<Derived*>(this)->arm();
    }

    /// @brief 返回累计处理字节数。
    /// @return 成功时返回总字节数，失败时返回 system error。
    auto await_resume() noexcept -> std::expected<resume_type, std::error_code>
    {
        if (error_code_ != 0)
            return unexpected_system_error(error_code_);

        return bytes_processed_;
    }

    /// @brief 单次 CQE 完成入口。
    ///
    /// 会先取消跟踪，再更新结果状态，最后决定是否继续 re-arm。
    void complete(int result, unsigned flags) noexcept override
    {
        context_->untrack(this);
        set_result(result, flags);
        finish_or_rearm(result, flags);
    }

    /// @brief 请求取消当前 operation。
    void cancel() noexcept
    {
        context_->cancel(this);
    }

    /// @brief 返回绑定的 IOContext。
    auto context() noexcept -> context_type&
    {
        return *context_;
    }

protected:
    /// @brief 根据 CQE 更新累计进度与错误状态。
    void set_result(int result, unsigned flags) noexcept
    {
        if (result > 0) {
            bytes_processed_ += static_cast<std::size_t>(result);
            buffer_ = buffer_.subspan(static_cast<std::size_t>(result));
        }
        else if (result == 0) {
            error_code_ = ECONNABORTED;
        }
        else {
            error_code_ = -result;
        }
    }

    /// @brief 判断收敛条件，或继续提交下一段请求。
    void finish_or_rearm(int result, unsigned flags) noexcept
    {
        if (is_canceling || error_code_ != 0 || buffer_.empty()) {
            if (is_canceling && error_code_ == 0)
                error_code_ = ECANCELED;

            resume(handle_, result, flags);
        }
        else if (!static_cast<Derived*>(this)->arm()) {
            resume(handle_, 0, 0);
        }
    }

    context_type* context_;
    Span buffer_;

    std::coroutine_handle<> handle_;
    std::size_t bytes_processed_{ 0 };
    int error_code_{ 0 };
};

} // namespace xin::async