export module xin.utility.mpsc_queue;

import std;

export namespace xin {

/// @brief MPSC 队列节点基类，提供无锁链接指针。
struct MPSCQueueNode {
    std::atomic<MPSCQueueNode*> mpsc_next{ nullptr };

    MPSCQueueNode() = default;

    MPSCQueueNode(MPSCQueueNode&& other) noexcept
      : mpsc_next{ nullptr }
    {}

    auto operator=(MPSCQueueNode&& other) noexcept -> MPSCQueueNode&
    {
        mpsc_next = nullptr;
        return *this;
    }
};

/// @brief 判断类型是否可作为 `MPSCQueue` 的节点。
/// @tparam T 待检测类型。
template<typename T>
concept mpsc_queue_node = std::derived_from<T, MPSCQueueNode>;

/// @brief 多生产者单消费者（MPSC）无锁队列。
///
/// 多个生产者可并发调用 `push()`，单个消费者通过 `pop_all()` 一次性取走当前全部节点。
/// `pop_all()` 返回的链表顺序与原始入队顺序一致。
///
/// ```cpp
/// struct Node : xin::MPSCQueueNode {
///     int value{};
/// };
///
/// xin::MPSCQueue<Node> queue;
/// Node first{ .value = 1 };
/// Node second{ .value = 2 };
/// queue.push(&first);
/// queue.push(&second);
/// auto* list = queue.pop_all();
/// ```
///
/// @tparam Node 节点类型，必须派生自 `MPSCQueueNode`。
template<mpsc_queue_node Node>
class MPSCQueue {
public:
    MPSCQueue() = default;

    MPSCQueue(const MPSCQueue&) = delete;
    auto operator=(const MPSCQueue&) -> MPSCQueue& = delete;

    MPSCQueue(MPSCQueue&&) = delete;
    auto operator=(MPSCQueue&&) -> MPSCQueue& = delete;

    ~MPSCQueue() = default;

    /// @brief 将节点推入队列。
    /// @param[in] node 待入队节点，生命周期必须覆盖被消费前的整个阶段。
    /// @return 若入队前队列为空则返回 `true`，否则返回 `false`。
    auto push(Node* node) noexcept -> bool
    {
        auto* expected = head_.load(std::memory_order_relaxed);
        do {
            node->mpsc_next.store(expected, std::memory_order_relaxed);
        } while (!head_.compare_exchange_weak(expected, node, std::memory_order_release,
                                              std::memory_order_relaxed));

        return expected == nullptr;
    }

    [[nodiscard]]
    auto empty() const noexcept -> bool
    {
        return head_.load(std::memory_order_acquire) == nullptr;
    }

    /// @brief 取出当前全部节点，并按入队顺序返回链表头。
    /// @return 队列为空时返回 `nullptr`，否则返回链表头节点。
    auto pop_all() noexcept -> Node*
    {
        auto* list = static_cast<Node*>(head_.exchange(nullptr, std::memory_order_acquire));
        if (!list)
            return nullptr;

        Node* prev = nullptr;
        while (list) {
            auto* next = static_cast<Node*>(list->mpsc_next.load(std::memory_order_relaxed));
            list->mpsc_next.store(prev, std::memory_order_relaxed);
            prev = list;
            list = next;
        }
        return prev;
    }

private:
    std::atomic<MPSCQueueNode*> head_{ nullptr };
};

} // namespace xin