module;

#include <cerrno>

export module xin.async.stop_then;

import std;

import xin.async.io_context;
import xin.async.operation;
import xin.async.post;
import xin.async.single_shot_operation;
import xin.utility;


export namespace xin::async {

/// @brief 为 single-shot operation 增加 `std::stop_token` 取消语义。
///
/// 该包装器会在 stop 被请求时，通过 `post(IOContext, ...)` 将取消动作
/// 投递到目标上下文线程，避免直接在 stop 回调线程里触碰 operation。
template<single_shot_operation Op>
class StopTokenWrapper : public Operation {
public:
    using resume_type = typename Op::resume_type;

    /// @brief 构造 stop_token 包装器。
    /// @param[in] op 被包装的 single-shot operation。
    /// @param[in] token 用于触发取消的 stop_token。
    StopTokenWrapper(Op&& op, std::stop_token token)
      : inner_{ std::forward<Op>(op) }
      , stop_token_{ std::move(token) }
    {}

    /// @brief 析构时关闭活跃标记并释放 stop_callback。
    ~StopTokenWrapper() override
    {
        *alive_ = false;
        stop_callback_.reset();
    }

    /// @brief 提前检查 stop_token。
    /// @return 若已请求 stop，则直接就绪并在 await_resume() 中返回取消错误。
    [[nodiscard]]
    auto await_ready() noexcept -> bool
    {
        if (stop_token_.stop_requested()) {
            result_ = -ECANCELED;
            return true;
        }

        return false;
    }

    /// @brief 挂起当前协程并注册 stop_callback。
    ///
    /// stop 请求到来后，会先投递到 `inner_.context()`，再在上下文线程中
    /// 调用 `inner_.cancel()`。
    auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool
    {
        handle_ = handle;
        inner_.parent = this;

        stop_callback_.emplace(std::move(stop_token_),
                               StopDispatch{ alive_, &inner_.context(), &inner_ });

        if (!inner_.await_suspend(handle)) {
            stop_callback_.reset();
            return false;
        }

        return true;
    }

    /// @brief 解析最终结果。
    /// @return stop 已触发时返回 `operation_canceled`；否则转发 inner 结果。
    auto await_resume() -> std::expected<resume_type, std::error_code>
    {
        if (result_ == -ECANCELED)
            return unexpected_system_error(std::errc::operation_canceled);

        return inner_.await_resume();
    }

    auto context() noexcept -> decltype(auto)
    {
        return inner_.context();
    }

    /// @brief inner 完成后恢复上层协程。
    void complete(int result, unsigned flags) noexcept override
    {
        *alive_ = false;
        stop_callback_.reset();

        result_ = result;
        flags_ = flags;

        this->resume(handle_, result, flags);
    }

    /// @brief 转发取消请求到 inner operation。
    void cancel() noexcept
    {
        inner_.cancel();
    }

private:
    /// @brief stop 回调中真正执行取消的函数对象。
    struct CancelDispatch {
        std::shared_ptr<bool> alive;
        Op* target;

        /// @brief 若包装器仍存活，则取消目标 operation。
        void operator()() const
        {
            if (*alive)
                target->cancel();
        }
    };

    /// @brief 将取消动作投递到目标 `IOContext` 的函数对象。
    struct StopDispatch {
        std::shared_ptr<bool> alive;
        IOContext* context;
        Op* target;

        /// @brief 先 post 到目标上下文，再由上下文线程执行取消。
        void operator()() const
        {
            post(*context, CancelDispatch{ alive, target });
        }
    };

    Op inner_;
    std::stop_token stop_token_;
    std::optional<std::stop_callback<std::function<void()>>> stop_callback_;
    std::shared_ptr<bool> alive_{ std::make_shared<bool>(true) };

    std::coroutine_handle<> handle_;
    int result_{ 0 };
    unsigned flags_{ 0 };
};

/// @brief 为 single-shot operation 创建 stop-token 包装器。
/// @param[in] operation 被包装的操作。
/// @param[in] token stop_token。
/// @return `StopTokenWrapper`。
template<single_shot_operation Op>
auto stop_then(Op&& operation, std::stop_token token)
{
    return StopTokenWrapper<std::decay_t<Op>>{ std::forward<Op>(operation), std::move(token) };
}

} // namespace xin::async