module;

#include <liburing.h>
#include <sys/eventfd.h>
#include <sys/poll.h>

export module xin.async.io_context;

import std;

import xin.async.awaiter;
import xin.utility;


export namespace xin::async {

class IOContext {
private:
    static constexpr std::uint64_t WAKEUP_MARKER = 1ULL << 63;
    static constexpr std::uint64_t CANCEL_MARKER = 1ULL << 62;

    ::io_uring ring_;
    std::thread::id thread_id_;
    int wakeup_fd_{ -1 };

    // 由于IOcontext的事件循环是单线程的，因此tracking_operations_不需要使用原子操作，直接使用普通的std::size_t即可
    std::size_t tracking_operations_{ 0 };
    std::atomic<bool> should_stop_{ false };

    Awaiter* head_{ nullptr };
    Awaiter* tail_{ nullptr };

    MPSCQueue<Awaiter> cross_thread_awaiters_;
    std::vector<Awaiter*> local_awaiters_;

    void process_cross_thread_awaiters() noexcept
    {
        auto* awaiter = cross_thread_awaiters_.pop_all();
        while (awaiter) {
            auto* next = static_cast<Awaiter*>(awaiter->mpsc_next.load(std::memory_order_relaxed));
            awaiter->resume(awaiter->result, awaiter->flags);
            awaiter = next;
        }
    }

    void process_local_awaiters() noexcept
    {
        std::vector<Awaiter*> awaiters;
        std::swap(awaiters, local_awaiters_);

        for (auto* awaiter : awaiters)
            awaiter->resume(awaiter->result, awaiter->flags);
    }

    void wakeup() const noexcept
    {
        uint64_t one = 1;
        ::write(wakeup_fd_, &one, sizeof(one));
    }

    void resume_wakeup() const noexcept
    {
        uint64_t buffer;
        ::read(wakeup_fd_, &buffer, sizeof(buffer));
    }

    void schedule()
    {
        process_local_awaiters();
        process_cross_thread_awaiters();

        if (tracking_operations_ == 0)
            return;

        auto res = ::io_uring_submit_and_wait(&ring_, 1);
        if (res < 0) {
            if (res == -EINTR)
                return;

            throw_system_error(-res, "io_uring_submit_and_wait failed");
        }

        unsigned count = 0;
        unsigned head;
        ::io_uring_cqe* cqe{ nullptr };

        io_uring_for_each_cqe(&ring_, head, cqe)
        {
            ++count;

            if (::io_uring_cqe_get_data64(cqe) == WAKEUP_MARKER) {
                resume_wakeup();
                break;
            }

            if (::io_uring_cqe_get_data64(cqe) == CANCEL_MARKER)
                continue;

            if (::io_uring_cqe_get_data64(cqe) != 0) {
                auto* awaiter = static_cast<Awaiter*>(::io_uring_cqe_get_data(cqe));
                untrack(awaiter);
                awaiter->resume(cqe->res, cqe->flags);
            }
        }

        if (count > 0)
            ::io_uring_cq_advance(&ring_, count);
    }

public:
    explicit IOContext(unsigned entries = 256)
    {
        constexpr auto flags = IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;
        if (auto res = ::io_uring_queue_init(entries, &ring_, flags); res < 0)
            throw_system_error(-res, "io_uring_queue_init failed");

        wakeup_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (wakeup_fd_ < 0)
            throw_system_error("created eventfd failed");

        auto* wakeup_sqe = sqe();
        ::io_uring_prep_poll_multishot(wakeup_sqe, wakeup_fd_, POLLIN);
        ::io_uring_sqe_set_data64(wakeup_sqe, WAKEUP_MARKER);
        ::io_uring_submit(&ring_);
    }

    ~IOContext()
    {
        ::io_uring_queue_exit(&ring_);
    }

    void run()
    {
        thread_id_ = std::this_thread::get_id();

        while (tracking_operations_ > 0) {
            if (should_stop_.load(std::memory_order_relaxed)) {
                // 取消所有未完成的 awaiter
                for (auto* awaiter = head_; awaiter; awaiter = awaiter->next)
                    cancel(*awaiter);
            }

            schedule();
        }
    }

    void stop() noexcept
    {
        should_stop_.store(true, std::memory_order_relaxed);
        wakeup();
    }

    auto ring() const noexcept -> const ::io_uring*
    {
        return &ring_;
    }

    // 将跨线程的 awaiter 加入队列，投递过来的 awaiter 本身已经携带结果和标志
    void post(Awaiter* awaiter) noexcept
    {
        if (cross_thread_awaiters_.push(awaiter))
            wakeup();
    }

    void dispatch(Awaiter* awaiter) noexcept
    {
        if (!is_owner_thread()) {
            post(awaiter);
            return;
        }

        local_awaiters_.push_back(awaiter);
    }

    void track(Awaiter* awaiter = nullptr) noexcept
    {
        if (awaiter) {
            if (!head_) {
                head_ = tail_ = awaiter;
            }
            else {
                tail_->next = awaiter;
                awaiter->prev = tail_;
                tail_ = awaiter;
            }
        }

        ++tracking_operations_;
    }

    void untrack(Awaiter* awaiter = nullptr) noexcept
    {
        if (awaiter) {
            if (awaiter->prev)
                awaiter->prev->next = awaiter->next;
            else
                head_ = awaiter->next;

            if (awaiter->next)
                awaiter->next->prev = awaiter->prev;
            else
                tail_ = awaiter->prev;

            awaiter->prev = awaiter->next = nullptr;
        }

        --tracking_operations_;
    }

    void cancel(Awaiter& awaiter) noexcept
    {
        if (awaiter.is_cancelled)
            return;

        if (auto* cancel_sqe = sqe()) {
            ::io_uring_prep_cancel(cancel_sqe, &awaiter, 0);
            ::io_uring_sqe_set_data64(cancel_sqe, CANCEL_MARKER);
            awaiter.is_cancelled = true;
        }
    }

    auto sqe() noexcept -> ::io_uring_sqe*
    {
        auto* sqe = ::io_uring_get_sqe(&ring_);
        if (!sqe) {
            ::io_uring_submit(&ring_);
            return ::io_uring_get_sqe(&ring_);
        }

        return sqe;
    }

    auto is_owner_thread() const noexcept -> bool
    {
        return std::this_thread::get_id() == thread_id_;
    }
};

} // namespace xin::async