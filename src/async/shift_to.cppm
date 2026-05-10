export module xin.async.shift_to;

import std;

import xin.async.io_context;
import xin.async.operation;


export namespace xin::async {

class ShiftToOperation final : public Operation {
public:
    explicit ShiftToOperation(std::coroutine_handle<> handle) noexcept
      : handle_{ handle }
    {}

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

class ShiftToAwaiter {
public:
    explicit ShiftToAwaiter(IOContext& context) noexcept
      : context_{ &context }
    {}

    [[nodiscard]]
    constexpr auto await_ready() const noexcept -> bool
    {
        return false;
    }

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

auto shift_to(IOContext& context) noexcept -> ShiftToAwaiter
{
    return ShiftToAwaiter{ context };
}

} // namespace xin::async
