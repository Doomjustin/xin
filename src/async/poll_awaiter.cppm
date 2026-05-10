module;

#include <liburing.h>

export module xin.async.poll_awaiter;

import std;

import xin.async.io_context;
import xin.async.single_shot_operation;
import xin.utility;


export namespace xin::async {

/// @brief 基于 io_uring poll_add 的单次等待器。
///
/// 该 awaiter 会监听指定 fd 的事件掩码，事件到达后恢复协程。
class PollAwaiter : public SingleShotOperation<PollAwaiter, void> {
public:
    /// @brief 构造 poll awaiter。
    /// @param[in] context 绑定的 IOContext。
    /// @param[in] fd 待监听的文件描述符。
    /// @param[in] events 事件掩码（如 `POLLIN` / `POLLOUT`）。
    PollAwaiter(context_type& context, int fd, short events)
      : SingleShotOperation<PollAwaiter, void>{ context }
      , fd_{ fd }
      , events_{ events }
    {}

    /// @brief 填充 poll_add SQE。
    void prepare(::io_uring_sqe* sqe) noexcept
    {
        ::io_uring_prep_poll_add(sqe, fd_, events_);
    }

private:
    int fd_;
    short events_;
};

} // namespace xin::async