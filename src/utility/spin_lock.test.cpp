#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.spin_lock;

namespace {

auto increment_with_lock(xin::SpinLock& lock, int& counter, int iterations) -> void
{
    for (int i = 0; i < iterations; ++i) {
        std::lock_guard guard{ lock };
        ++counter;
    }
}

} // namespace

TEST_CASE("xin::SpinLock 的 try_lock 与 unlock 行为正确", "[utility][spin_lock]")
{
    xin::SpinLock lock;

    REQUIRE(lock.try_lock());
    REQUIRE_FALSE(lock.try_lock());

    lock.unlock();

    REQUIRE(lock.try_lock());
    lock.unlock();
}

TEST_CASE("xin::SpinLock 支持 lock_guard 管理临界区", "[utility][spin_lock]")
{
    xin::SpinLock lock;
    int value = 0;

    {
        std::lock_guard guard{ lock };
        value = 42;
        REQUIRE_FALSE(lock.try_lock());
    }

    REQUIRE(value == 42);
    REQUIRE(lock.try_lock());
    lock.unlock();
}

TEST_CASE("xin::SpinLock 在多线程下保护共享计数", "[utility][spin_lock]")
{
    xin::SpinLock lock;
    int counter = 0;
    constexpr int thread_count = 4;
    constexpr int iterations = 2000;

    std::vector<std::jthread> threads;
    threads.reserve(thread_count);

    for (int index = 0; index < thread_count; ++index)
        threads.emplace_back(increment_with_lock, std::ref(lock), std::ref(counter), iterations);

    threads.clear();

    REQUIRE(counter == thread_count * iterations);
}
