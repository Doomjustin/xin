module;

#include <liburing.h>

export module xin.async.single_shot_operation;

import std;

import xin.async.io_context;
import xin.async.operation;
import xin.utility;


export namespace xin::async {

/// @brief 单次提交（single-shot）IO 操作基类。
///
/// 派生类需提供：
/// - `prepare(io_uring_sqe*)`：填充 SQE；
/// - 非 `void` 返回场景下的 `result()` / `set_result(...)`。
///
/// 基类负责：
/// - 协程挂起与恢复；
/// - 向 context 跟踪/摘除 operation；
/// - `error_code` 到 `expected` 的统一转换。
template<typename Derived, typename Resume>
class SingleShotOperation : public Operation {
public:
    using is_single_shot = std::true_type;
    using resume_type = Resume;
    using context_type = IOContext;

    /// @brief 构造 single-shot 操作。
    /// @param[in] context 绑定的 IOContext。
    SingleShotOperation(context_type& context)
      : context_{ &context }
    {}

    [[nodiscard]]
    constexpr auto await_ready() const noexcept -> bool
    {
        return false;
    }

    /// @brief 挂起协程并提交一次 SQE。
    /// @return 提交成功返回 `true`；SQE 不足返回 `false` 并设置 `EAGAIN`。
    auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool
    {
        this->handle_ = handle;

        if (auto* sqe = context_->sqe()) {
            static_cast<Derived*>(this)->prepare(sqe);
            ::io_uring_sqe_set_data(sqe, this);

            context_->track(this);
            return true;
        }

        error_code_ = EAGAIN;
        return false;
    }

    /// @brief 获取 single-shot 执行结果。
    /// @return 成功时返回 `resume_type` 对应结果；失败时返回 system error。
    auto await_resume() noexcept -> std::expected<resume_type, std::error_code>
    {
        if (error_code_ != 0)
            return unexpected_system_error(error_code_);

        if constexpr (std::is_void_v<resume_type>)
            return {};
        else
            return static_cast<Derived*>(this)->result();
    }

    /// @brief CQE 完成回调。
    ///
    /// 先从 context 摘除，再更新错误/结果，最后恢复协程。
    void complete(int result, unsigned flags) noexcept override
    {
        context_->untrack(this);

        if (result < 0)
            error_code_ = -result;
        else if constexpr (!std::is_void_v<resume_type>)
            static_cast<Derived*>(this)->set_result(result, flags);

        this->resume(handle_, result, flags);
    }

    /// @brief 转发取消请求到 context。
    void cancel() noexcept
    {
        context_->cancel(this);
    }

    /// @brief 获取绑定的 IOContext。
    auto context() noexcept -> context_type&
    {
        return *context_;
    }

protected:
    context_type* context_;
    std::coroutine_handle<> handle_{ nullptr };
    int error_code_{ 0 };
};

/// @brief 判断类型是否满足 single-shot operation 约束。
template<typename T>
concept single_shot_operation = requires {
    typename T::is_single_shot;
    requires std::same_as<typename T::is_single_shot, std::true_type>;
};

} // namespace xin::async