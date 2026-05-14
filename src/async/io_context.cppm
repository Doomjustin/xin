module;

#include <liburing.h>
#include <liburing/io_uring.h>
#include <sys/eventfd.h>
#include <sys/poll.h>

export module xin.async.io_context;

import std;

import xin.async.awaiter;
import xin.utility;


export namespace xin::async {

/// @brief 协程恢复执行器。
///
/// 负责处理两类恢复请求：
/// - 同线程 `dispatch` 的本地 awaiter。
/// - 跨线程 `post` 的 awaiter。
class Executor {
private:
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

public:
    /// @brief 将跨线程 awaiter 投递到 MPSC 队列。
    /// @param[in] awaiter 待恢复的 awaiter，需已携带 result/flags。
    /// @return true 表示队列由空转非空，调用方通常需要触发 wakeup。
    auto post(Awaiter* awaiter) noexcept -> bool
    {
        return cross_thread_awaiters_.push(awaiter);
    }

    /// @brief 将同线程 awaiter 放入本地恢复队列。
    /// @param[in] awaiter 待恢复的 awaiter。
    void dispatch(Awaiter* awaiter) noexcept
    {
        local_awaiters_.push_back(awaiter);
    }

    /// @brief 执行一次恢复调度。
    ///
    /// 顺序为：先处理本地队列，再处理跨线程队列。
    void execute() noexcept
    {
        process_local_awaiters();
        process_cross_thread_awaiters();
    }
};

/// @brief 基于 io_uring 的事件循环上下文。
///
/// 该类型提供：
/// - SQE/CQE 驱动的 I/O completion 调度；
/// - 以 `id` 为键的 pending awaiter 跟踪；
/// - 跨线程取消请求的 owner-thread 串行化提交。
class IOContext {
private:
    /// @brief 跨线程取消请求节点，仅携带 awaiter id。
    struct CancelNode : public MPSCQueueNode {
        std::uint64_t id;

        CancelNode(std::uint64_t id) noexcept
          : id{ id }
        {}
    };

    static constexpr std::uint64_t WAKEUP_MARKER = 1ULL << 63; // 100...0, 用于标记唤醒事件
    static constexpr std::uint64_t CANCEL_MARKER = 1ULL << 62; // 010...0, 用于标记取消事件

    ::io_uring ring_;
    std::thread::id thread_id_;
    int wakeup_fd_{ -1 };

    std::atomic<std::size_t> tracking_works_{ 0 };
    std::atomic<bool> should_stop_{ false };

    Executor executor_;

    std::uint64_t next_id_{ 1 };
    // WARN: 这里的 unordered_map 可能会成为性能瓶颈，后续可以考虑使用更高效的
    // ID 分配与存储方案，如分段锁定哈希表或 ID 池。
    std::unordered_map<std::uint64_t, Awaiter*> pending_tasks_;
    // 线程安全的取消请求队列：跨线程仅入队 ID，由 owner 线程统一发起 cancel SQE。
    MPSCQueue<CancelNode> cancel_queue_;

    /// @brief 生成 awaiter id，保留高两位给内部 marker。
    /// @return 可用于 `io_uring_sqe_set_data64` 的 user_data。
    auto generate_id() noexcept -> std::uint64_t
    {
        return next_id_++ & 0x3FFFFFFFFFFFFFFF; // 保持最高2位为0，避免与特殊标记冲突
    }

    /// @brief 跨线程入队取消请求并唤醒 owner 线程。
    /// @param[in] awaiter 目标 awaiter。
    void enqueue_cancel(Awaiter* awaiter) noexcept
    {
        auto* node = new CancelNode{ awaiter->id };
        cancel_queue_.push(node);
        wakeup();
    }

    /// @brief 为指定 id 准备 cancel SQE。
    /// @param[in] id 待取消的 awaiter id。
    /// @return true 表示成功拿到 SQE 并完成填充。
    auto prepare_cancel_sqe(std::uint64_t id) noexcept -> bool
    {
        if (auto* cancel_sqe = sqe()) {
            ::io_uring_prep_cancel64(cancel_sqe, id, 0);
            ::io_uring_sqe_set_data64(cancel_sqe, CANCEL_MARKER);
            return true;
        }

        return false;
    }

    /// @brief 在 owner 线程尝试取消指定 id。
    /// @param[in] id 待取消的 awaiter id。
    ///
    /// 若 id 已不在 pending 集合，表示任务已完成或已被移除，本次取消会被忽略。
    void cancel(std::uint64_t id) noexcept
    {
        // 如果id不存在，说明任务可能完成也可能已经被取消，无需重复提交取消请求。
        if (pending_tasks_.contains(id) && prepare_cancel_sqe(id))
            ::io_uring_submit(&ring_);
    }

    /// @brief 消费跨线程取消队列并提交 cancel SQE。
    ///
    /// 该函数仅在 owner 线程执行，确保 `ring_` 的提交路径串行化。
    void process_cancel() noexcept
    {
        auto* node = cancel_queue_.pop_all();
        while (node) {
            prepare_cancel_sqe(node->id);
            ::io_uring_submit(&ring_);
            auto* next = static_cast<CancelNode*>(node->mpsc_next.load(std::memory_order_relaxed));
            delete node;
            node = next;
        }
    }

    /// @brief 为 `wakeup_fd_` 注册 multishot poll。
    void arm_wakeup() noexcept
    {
        auto* wakeup_sqe = sqe();
        ::io_uring_prep_poll_multishot(wakeup_sqe, wakeup_fd_, POLLIN);
        ::io_uring_sqe_set_data64(wakeup_sqe, WAKEUP_MARKER);
    }

    /// @brief 向 wakeup fd 写入事件，用于唤醒 owner 线程。
    void wakeup() const noexcept
    {
        uint64_t one = 1;
        ::write(wakeup_fd_, &one, sizeof(one));
    }

    /// @brief 读取并清空 wakeup fd 的可读状态。
    void resume_wakeup() const noexcept
    {
        uint64_t buffer;
        ::read(wakeup_fd_, &buffer, sizeof(buffer));
    }

    /// @brief 执行一次调度循环。
    ///
    /// 顺序：执行恢复队列 -> 处理取消队列 -> 等待并消费 CQE。
    void schedule()
    {
        executor_.execute();
        process_cancel();

        if (tracking_works_.load(std::memory_order_relaxed) == 0)
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

            auto user_data = ::io_uring_cqe_get_data64(cqe);

            if (user_data == WAKEUP_MARKER) {
                resume_wakeup();
                if (!(cqe->flags & IORING_CQE_F_MORE))
                    arm_wakeup();

                continue;
            }

            if (user_data == CANCEL_MARKER)
                continue;

            if (user_data != 0) {
                if (auto it = pending_tasks_.find(user_data); it != pending_tasks_.end()) {
                    auto* awaiter = it->second;
                    untrack(it->second);
                    awaiter->resume(cqe->res, cqe->flags);
                }
            }
        }

        if (count > 0)
            ::io_uring_cq_advance(&ring_, count);
    }

public:
    /// @brief 构造 IOContext 并初始化 io_uring / wakeup 通道。
    /// @param[in] entries SQ/CQ 初始容量。
    explicit IOContext(unsigned entries = 256)
    {
        if (auto res = ::io_uring_queue_init(entries, &ring_, 0); res < 0)
            throw_system_error(-res, "io_uring_queue_init failed");

        wakeup_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (wakeup_fd_ < 0)
            throw_system_error("created eventfd failed");

        arm_wakeup();
        ::io_uring_submit(&ring_);
    }

    ~IOContext()
    {
        ::io_uring_queue_exit(&ring_);

        if (wakeup_fd_ != -1)
            ::close(wakeup_fd_);
    }

    /// @brief 进入事件循环直到 tracked work 归零。
    void run()
    {
        thread_id_ = std::this_thread::get_id();

        while (tracking_works_.load(std::memory_order_relaxed) > 0) {
            if (should_stop_.load(std::memory_order_relaxed)) {
                // 取消所有未完成的 awaiter
                for (auto& [id, _] : pending_tasks_)
                    prepare_cancel_sqe(id);

                ::io_uring_submit(&ring_);
                should_stop_.store(false, std::memory_order_relaxed);
            }

            schedule();
        }
    }

    /// @brief 请求停止并唤醒事件循环线程。
    void stop() noexcept
    {
        should_stop_.store(true, std::memory_order_relaxed);
        wakeup();
    }

    /// @brief 获取底层 io_uring 只读句柄。
    auto ring() const noexcept -> const ::io_uring*
    {
        return &ring_;
    }

    /// @brief 跨线程投递恢复请求。
    /// @param[in] awaiter 待恢复的 awaiter。
    void post(Awaiter* awaiter) noexcept
    {
        if (executor_.post(awaiter))
            wakeup();
    }

    /// @brief 同线程投递恢复请求。
    /// @param[in] awaiter 待恢复的 awaiter。
    void dispatch(Awaiter* awaiter) noexcept
    {
        executor_.dispatch(awaiter);
    }

    /// @brief 追踪一个新的工作项并可选绑定 SQE user_data。
    /// @param[in,out] sqe 若非空则写入 awaiter id 到 user_data。
    /// @param[in] awaiter 若非空则为其分配 id 并加入 pending 集合。
    void track(::io_uring_sqe* sqe = nullptr, Awaiter* awaiter = nullptr) noexcept
    {
        if (awaiter) {
            awaiter->id = generate_id();
            pending_tasks_.emplace(awaiter->id, awaiter);

            if (sqe)
                ::io_uring_sqe_set_data64(sqe, awaiter->id);
        }

        tracking_works_.fetch_add(1, std::memory_order_relaxed);
    }

    /// @brief 取消追踪一个工作项。
    /// @param[in] awaiter 若非空则从 pending 集合移除对应 id。
    void untrack(Awaiter* awaiter = nullptr) noexcept
    {
        if (awaiter)
            pending_tasks_.erase(awaiter->id);

        tracking_works_.fetch_sub(1, std::memory_order_relaxed);
    }

    /// @brief 请求取消一个 awaiter。
    /// @param[in] awaiter 目标 awaiter。
    ///
    /// owner 线程直接提交 cancel；非 owner 线程走 cancel_queue_。
    void cancel(Awaiter& awaiter) noexcept
    {
        if (is_owner_thread())
            cancel(awaiter.id);
        else
            enqueue_cancel(&awaiter);
    }

    /// @brief 获取一个可写 SQE，必要时先 submit 以腾出空间。
    auto sqe() noexcept -> ::io_uring_sqe*
    {
        auto* sqe = ::io_uring_get_sqe(&ring_);
        if (!sqe) {
            ::io_uring_submit(&ring_);
            return ::io_uring_get_sqe(&ring_);
        }

        return sqe;
    }

    /// @brief 判断当前线程是否为事件循环 owner 线程。
    auto is_owner_thread() const noexcept -> bool
    {
        return std::this_thread::get_id() == thread_id_;
    }
};

} // namespace xin::async