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

/// @brief 基于 io_uring 的异步执行上下文。
///
/// `IOContext` 负责：
/// - 管理 SQE/CQE 调度循环；
/// - 跟踪活动 operation 的生命周期；
/// - 提供跨线程投递与 owner-thread 内快速提交。
class IOContext {
public:
    /// @brief buffer ring 元信息。
    struct BufferRing {
        void* base_address{ nullptr };
        unsigned size{ 0 };
        unsigned mask{ 0 };
        unsigned entries{ 0 };
        std::uint16_t tail{ 0 };
        ::io_uring_buf_ring* buffer{ nullptr };
    };

    /// @brief 构造 IOContext。
    /// @param[in] entries io_uring 队列深度。
    explicit IOContext(unsigned entries = 1024)
      : scheduler_{ entries }
    {}

    IOContext(const IOContext&) = delete;
    auto operator=(const IOContext&) -> IOContext& = delete;

    IOContext(IOContext&& other) noexcept = delete;
    auto operator=(IOContext&&) -> IOContext& = delete;

    ~IOContext() = default;

    /// @brief 运行事件循环直到 work 计数归零。
    void run();

    /// @brief 请求停止事件循环。
    void stop();

    [[nodiscard]]
    /// @brief 获取可写 SQE。
    auto sqe() noexcept -> ::io_uring_sqe*
    {
        return scheduler_.sqe();
    }

    /// @brief 获取底层 io_uring 句柄。
    auto ring() noexcept -> ::io_uring*
    {
        return scheduler_.ring();
    }

    [[nodiscard]]
    /// @brief 获取只读底层 io_uring 句柄。
    auto ring() const noexcept -> const ::io_uring*
    {
        return scheduler_.ring();
    }

    /// @brief 跟踪 operation 生命周期并增加 work。
    void track(gsl::not_null<Operation*> operation) noexcept;
    /// @brief 摘除 operation 生命周期并减少 work。
    void untrack(gsl::not_null<Operation*> operation) noexcept;

    /// @brief 提交取消请求。
    void cancel(gsl::not_null<Operation*> operation) noexcept;

    /// @brief work 计数加一。
    void add_work() noexcept
    {
        tracking_operations_.fetch_add(1, std::memory_order_relaxed);
    }

    /// @brief work 计数减一。
    void drop_work() noexcept
    {
        auto prev = tracking_operations_.fetch_sub(1, std::memory_order_relaxed);
        assert(prev > 0);
    }

    [[nodiscard]]
    /// @brief 创建 buffer ring。
    /// @return 新分配的 bgid。
    auto setup_buffer_ring(unsigned entries, unsigned size) -> unsigned
    {
        return buffers_.setup(scheduler_.ring(), entries, size);
    }

    /// @brief 归还一个 buffer 到 ring。
    void release_buffer_ring(unsigned bgid, unsigned bid)
    {
        buffers_.release(bgid, bid);
    }

    /// @brief 设置默认 buffer group。
    void set_default_buffer(unsigned bgid)
    {
        buffers_.set_default_buffer(bgid);
    }

    /// @brief 获取默认 buffer group。
    auto default_buffer() -> std::optional<unsigned>
    {
        return buffers_.default_buffer();
    }

    /// @brief 获取指定 bgid 的 buffer ring 元信息。
    auto buffer_ring(unsigned bgid) -> BufferRing&
    {
        return buffers_.buffer_ring(bgid);
    }

    /// @brief 跨线程投递 operation。
    void post(gsl::not_null<Operation*> operation) noexcept
    {
        scheduler_.post(operation);
    }

    /// @brief 在 owner thread 本地提交 operation。
    void submit(gsl::not_null<Operation*> operation) noexcept
    {
        scheduler_.submit(operation);
    }

    [[nodiscard]]
    /// @brief 判断当前线程是否为 owner thread。
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

    Operation* head_{ nullptr };
    Operation* tail_{ nullptr };
    std::thread::id thread_id_;
    std::atomic<std::size_t> tracking_operations_{ 0 };
    std::atomic<bool> should_stop_{ false };
};

} // namespace xin::async