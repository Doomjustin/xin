module;

#include <cassert>

#include <liburing.h>
#include <sys/eventfd.h>
#include <sys/poll.h>

#include <gsl/gsl>

export module xin.async.io_context;

import std;

import xin.async.operation;
import xin.utility;


export namespace xin::async {

class IOContext {
public:
    struct BufferRing {
        void* base_address{ nullptr };
        unsigned size{ 0 };
        unsigned mask{ 0 };
        unsigned entries{ 0 };
        std::uint16_t tail{ 0 };
        ::io_uring_buf_ring* buffer{ nullptr };
    };

    explicit IOContext(unsigned entries = 1024)
      : scheduler_{ entries }
    {}

    IOContext(const IOContext&) = delete;
    auto operator=(const IOContext&) -> IOContext& = delete;

    IOContext(IOContext&& other) noexcept = delete;
    auto operator=(IOContext&&) -> IOContext& = delete;

    ~IOContext() = default;

    void run();

    void stop();

    [[nodiscard]]
    auto sqe() noexcept -> ::io_uring_sqe*
    {
        return scheduler_.sqe();
    }

    auto ring() noexcept -> ::io_uring*
    {
        return scheduler_.ring();
    }

    [[nodiscard]]
    auto ring() const noexcept -> const ::io_uring*
    {
        return scheduler_.ring();
    }

    void track(gsl::not_null<Operation*> operation) noexcept;
    void untrack(gsl::not_null<Operation*> operation) noexcept;

    void cancel(gsl::not_null<Operation*> operation) noexcept;

    void add_work() noexcept
    {
        tracking_operations_.fetch_add(1, std::memory_order_relaxed);
    }

    void drop_work() noexcept
    {
        auto prev = tracking_operations_.fetch_sub(1, std::memory_order_relaxed);
        assert(prev > 0);
    }

    [[nodiscard]]
    auto setup_buffer_ring(unsigned entries, unsigned size) -> unsigned
    {
        return buffers_.setup(scheduler_.ring(), entries, size);
    }

    void release_buffer_ring(unsigned bgid, unsigned bid)
    {
        buffers_.release(bgid, bid);
    }

    void set_default_buffer(unsigned bgid)
    {
        buffers_.set_default_buffer(bgid);
    }

    auto default_buffer() -> std::optional<unsigned>
    {
        return buffers_.default_buffer();
    }

    auto buffer_ring(unsigned bgid) -> BufferRing&
    {
        return buffers_.buffer_ring(bgid);
    }

    void post(gsl::not_null<Operation*> operation) noexcept
    {
        scheduler_.post(operation);
    }

    void submit(gsl::not_null<Operation*> operation) noexcept
    {
        scheduler_.submit(operation);
    }

    [[nodiscard]]
    auto is_owner_thread() const noexcept -> bool
    {
        return std::this_thread::get_id() == thread_id_;
    }

private:
    class Scheduler {
    public:
        explicit Scheduler(unsigned entries);

        ~Scheduler();

        void wakeup() const noexcept;

        auto sqe() -> ::io_uring_sqe*;

        void schedule(const std::atomic_size_t& tracking);

        void post(gsl::not_null<Operation*> operation) noexcept
        {
            // 仅在队列从0->1时才手动唤醒
            if (cross_thread_operations_.push(operation))
                wakeup();
        }

        void submit(gsl::not_null<Operation*> operation) noexcept
        {
            local_operations_.push_back(operation);
        }

        auto ring() noexcept -> ::io_uring*
        {
            return &ring_;
        }

        [[nodiscard]]
        auto ring() const noexcept -> const ::io_uring*
        {
            return &ring_;
        }

    private:
        struct PendingEvent {
            bool is_wakeup{ false };
            Operation* operation{ nullptr };
            int result{ 0 };
            std::uint32_t flags{ 0 };
        };

        static constexpr auto WAKEUP_MARKER = std::numeric_limits<std::uintptr_t>::max();

        ::io_uring ring_;
        int wakeup_fd_;
        ::io_uring_cqe* cqe_{ nullptr };

        MPSCQueue<Operation> cross_thread_operations_;
        std::vector<Operation*> local_operations_;
        std::vector<PendingEvent> pending_cqe_events_;

        void arm_wakeup();
        void resume_wakeup() const noexcept;
        void process_cross_thread_operations() noexcept;
        void process_local_operations() noexcept;
        void collect_cqe_events(std::vector<PendingEvent>& pending_events,
                                unsigned& count) noexcept;
        void dispatch_cqe_events(std::vector<PendingEvent>& pending_events) noexcept;
    };

    class BufferRingGroup {
    public:
        explicit BufferRingGroup(
            std::pmr::memory_resource* resource = std::pmr::get_default_resource())
          : memory_resource_{ resource }
        {}

        ~BufferRingGroup();

        auto setup(::io_uring* ring, unsigned entries, unsigned size) -> unsigned;
        void release(unsigned bgid, unsigned bid);

        void set_default_buffer(unsigned bgid);

        auto default_buffer() -> std::optional<unsigned>
        {
            return default_buffer_bgid_;
        }

        [[nodiscard]]
        constexpr auto empty() const noexcept -> bool
        {
            return next_bgid_ == INIT_BGID;
        }

        [[nodiscard]]
        constexpr auto size() const noexcept -> std::size_t
        {
            return static_cast<std::size_t>(next_bgid_);
        }

        auto buffer_ring(unsigned bgid) noexcept -> BufferRing&
        {
            assert(bgid < GROUP_SIZE);
            return group_[bgid];
        }

    private:
        static constexpr unsigned GROUP_SIZE = 16;
        static constexpr unsigned MAX_BGID{ GROUP_SIZE - 1 };
        static constexpr std::size_t ALIGNMENT = 4096;
        static constexpr unsigned INIT_BGID{ 0 };

        std::pmr::memory_resource* memory_resource_;
        ::io_uring* ring_;
        std::array<BufferRing, GROUP_SIZE> group_;
        unsigned next_bgid_{ INIT_BGID };
        std::optional<unsigned> default_buffer_bgid_;
    };

    Scheduler scheduler_;
    BufferRingGroup buffers_;

    Operation* head_;
    Operation* tail_;
    std::thread::id thread_id_;
    std::atomic<std::size_t> tracking_operations_{ 0 };
    std::atomic<bool> should_stop_{ false };
};

} // namespace xin::async