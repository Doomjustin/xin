#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.mpsc_queue;

namespace {

struct node_type : xin::MPSCQueueNode {
    int value{};
};

auto values_from_list(node_type* head) -> std::vector<int>
{
    std::vector<int> values;

    for (auto* current = head; current != nullptr;
         current = static_cast<node_type*>(current->mpsc_next.load(std::memory_order_relaxed)))
        values.push_back(current->value);

    return values;
}

} // namespace

TEST_CASE("xin::MPSCQueue 初始为空", "[utility][mpsc_queue]")
{
    xin::MPSCQueue<node_type> queue;

    REQUIRE(queue.empty());
    REQUIRE(queue.pop_all() == nullptr);
}

TEST_CASE("xin::MPSCQueue push 在空队列上返回 true", "[utility][mpsc_queue]")
{
    xin::MPSCQueue<node_type> queue;
    node_type first{ .value = 1 };
    node_type second{ .value = 2 };

    REQUIRE(queue.push(&first));
    REQUIRE_FALSE(queue.push(&second));
    REQUIRE_FALSE(queue.empty());
}

TEST_CASE("xin::MPSCQueue pop_all 按入队顺序返回全部节点", "[utility][mpsc_queue]")
{
    xin::MPSCQueue<node_type> queue;
    node_type first{ .value = 1 };
    node_type second{ .value = 2 };
    node_type third{ .value = 3 };

    queue.push(&first);
    queue.push(&second);
    queue.push(&third);

    auto* head = queue.pop_all();

    REQUIRE(values_from_list(head) == std::vector<int>{ 1, 2, 3 });
    REQUIRE(queue.empty());
    REQUIRE(queue.pop_all() == nullptr);
}

TEST_CASE("xin::MPSCQueue 在再次入队后仍可继续消费", "[utility][mpsc_queue]")
{
    xin::MPSCQueue<node_type> queue;
    node_type first{ .value = 1 };
    node_type second{ .value = 2 };

    queue.push(&first);
    REQUIRE(values_from_list(queue.pop_all()) == std::vector<int>{ 1 });

    REQUIRE(queue.empty());
    REQUIRE(queue.push(&second));
    REQUIRE(values_from_list(queue.pop_all()) == std::vector<int>{ 2 });
}

TEST_CASE("xin::MPSCQueueNode move 后链接指针被重置", "[utility][mpsc_queue]")
{
    node_type next{ .value = 8 };
    node_type source{ .value = 7 };
    source.mpsc_next.store(&next, std::memory_order_relaxed);

    node_type moved{ std::move(source) };

    REQUIRE(moved.mpsc_next.load(std::memory_order_relaxed) == nullptr);

    node_type assigned{ .value = 9 };
    assigned.mpsc_next.store(&next, std::memory_order_relaxed);

    assigned = std::move(source);

    REQUIRE(assigned.mpsc_next.load(std::memory_order_relaxed) == nullptr);
}
