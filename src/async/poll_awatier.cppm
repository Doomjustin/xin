module;

#include <liburing.h>

export module xin.async.poll_awaiter;

import std;

import xin.async.single_shot_awaiter;


export namespace xin::async {

class PollAwaiter : public SingleShotAwaiter<PollAwaiter> {
private:
    int fd_;
    short events_;

public:
    PollAwaiter(int fd, short events) noexcept
      : fd_{ fd }
      , events_{ events }
    {}

    void prepare(::io_uring_sqe* sqe) const noexcept
    {
        ::io_uring_prep_poll_add(sqe, fd_, events_);
    }
};

} // namespace xin::async