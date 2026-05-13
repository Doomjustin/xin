export module xin.async.awaiter;

import std;

import xin.utility;


export namespace xin::async {

/// @brief 事件循环使用的基础 Awaiter 节点。
///
/// Awaiter 同时承担 coroutine continuation 与调度链表节点角色：
/// - `handle` 用于恢复当前 coroutine。
/// - `parent` 用于将 completion 结果向上游 Awaiter 传播。
/// - `prev/next` 用于挂接到 `IOContext` 的跟踪链表。
struct Awaiter : public MPSCQueueNode {
    std::coroutine_handle<> handle;
    Awaiter* parent{ nullptr };
    Awaiter* prev{ nullptr };
    Awaiter* next{ nullptr };

    int result;
    std::uint32_t flags;
    bool is_cancelled{ false };

    Awaiter() = default;

    Awaiter(const Awaiter&) = delete;
    auto operator=(const Awaiter&) -> Awaiter& = delete;

    Awaiter(Awaiter&&) = default;
    auto operator=(Awaiter&&) -> Awaiter& = default;

    virtual ~Awaiter() = default;

    /// @brief 处理 completion 并恢复 coroutine。
    /// @param[in] result completion 的结果码（与 io_uring CQE `res` 语义一致）。
    /// @param[in] flags completion 的附加标志（与 io_uring CQE `flags` 语义一致）。
    /// @return 无。
    virtual void resume(int result, std::uint32_t flags)
    {
        if (parent) {
            parent->resume(result, flags);
        }
        else {
            this->result = result;
            this->flags = flags;
            std::exchange(handle, nullptr).resume();
        }
    }
};

} // namespace xin::async