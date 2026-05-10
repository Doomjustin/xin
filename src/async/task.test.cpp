#include <catch2/catch_test_macros.hpp>

import std;

import xin.async.task;

namespace {

auto make_value_task() -> xin::async::Task<int>
{
    co_return 42;
}

auto consume_value_task() -> xin::async::Task<int>
{
    co_return co_await make_value_task();
}

auto make_throwing_task() -> xin::async::Task<int>
{
    throw std::runtime_error{ "task failure" };
    co_return 0;
}

auto observe_throwing_task() -> xin::async::Task<bool>
{
    try {
        (void)co_await make_throwing_task();
        co_return false;
    }
    catch (const std::runtime_error&) {
        co_return true;
    }
}

auto await_invalid_task() -> xin::async::Task<bool>
{
    xin::async::Task<int> task{};

    try {
        co_await std::move(task);
        co_return false;
    }
    catch (const std::logic_error&) {
        co_return true;
    }
}

} // namespace

TEST_CASE("xin::Task co_await 可获取返回值", "[async][task]")
{
    auto task = consume_value_task();

    REQUIRE_FALSE(task.done());

    task.handle().resume();

    REQUIRE(task.done());
    REQUIRE(task.handle().promise().result() == 42);
}

TEST_CASE("xin::Task 在 await_resume 时传播异常", "[async][task]")
{
    auto task = observe_throwing_task();

    task.handle().resume();

    REQUIRE(task.done());
    REQUIRE(task.handle().promise().result());
}

TEST_CASE("xin::Task 对无效句柄在 await_resume 抛出逻辑错误", "[async][task]")
{
    auto task = await_invalid_task();

    task.handle().resume();

    REQUIRE(task.done());
    REQUIRE(task.handle().promise().result());
}
