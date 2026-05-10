export module xin.async.operation;

import std;

import xin.utility;


export namespace xin::async {

struct Operation : public MPSCQueueNode {
    Operation* prev{ nullptr };
    Operation* next{ nullptr };

    // 用于取消
    Operation* parent{ nullptr };
    bool is_canceling{ false };

    // 用于完成时的结果传递
    // 只会在投递任务的场景使用，普通的io操作使用cqe的user_data传递结果
    int result{ 0 };

    Operation() = default;

    Operation(const Operation&) = delete;
    auto operator=(const Operation&) -> Operation& = delete;

    Operation(Operation&&) = default;
    auto operator=(Operation&&) -> Operation& = default;

    virtual ~Operation() = default;

    virtual void complete(int result, unsigned flags) noexcept = 0;

    void resume(std::coroutine_handle<> handle, int result, unsigned flags) const noexcept
    {
        if (parent)
            parent->complete(result, flags);
        else
            std::exchange(handle, nullptr).resume();
    }
};

template<typename T>
concept cancelable_operation = requires(T& t) {
    typename T::resume_type;

    requires std::is_lvalue_reference_v<decltype(t.context())>;
    t.cancel();
};

} // namespace xin::async