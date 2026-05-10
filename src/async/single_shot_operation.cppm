module;

#include <liburing.h>

export module xin.async.single_shot_operation;

import std;

import xin.async.io_context;
import xin.async.operation;
import xin.utility;


export namespace xin::async {

template<typename Derived, typename Resume>
class SingleShotOperation : public Operation {
public:
    using is_single_shot = std::true_type;
    using resume_type = Resume;
    using context_type = IOContext;

    SingleShotOperation(context_type& context)
      : context_{ &context }
    {}

    [[nodiscard]]
    constexpr auto await_ready() const noexcept -> bool
    {
        return false;
    }

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

    auto await_resume() noexcept -> std::expected<resume_type, std::error_code>
    {
        if (error_code_ != 0)
            return unexpected_system_error(error_code_);

        if constexpr (std::is_void_v<resume_type>)
            return {};
        else
            return static_cast<Derived*>(this)->result();
    }

    void complete(int result, unsigned flags) noexcept override
    {
        context_->untrack(this);

        if (result < 0)
            error_code_ = -result;
        else if constexpr (!std::is_void_v<resume_type>)
            static_cast<Derived*>(this)->set_result(result, flags);

        this->resume(handle_, result, flags);
    }

    void cancel() noexcept
    {
        context_->cancel(this);
    }

    auto context() noexcept -> context_type&
    {
        return *context_;
    }

protected:
    context_type* context_;
    std::coroutine_handle<> handle_{ nullptr };
    int error_code_{ 0 };
};

template<typename T>
concept single_shot_operation = requires {
    typename T::is_single_shot;
    requires std::same_as<typename T::is_single_shot, std::true_type>;
};

} // namespace xin::async