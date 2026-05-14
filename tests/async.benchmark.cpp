#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

import std;

import xin.async;


// 独立的全局协程，彻底消灭 Lambda 隐式捕获陷阱！
auto benchmark3_worker(std::atomic<int>& c, std::atomic_flag& d) -> xin::async::Task<>
{
    if (c.fetch_sub(1, std::memory_order_relaxed) == 1) {
        d.test_and_set(std::memory_order_release);
        d.notify_one();
    }
    co_return;
}

TEST_CASE("IOContext 吞吐量基准测试", "[benchmark][async]")
{
    // 测试 1：单线程全链路
    BENCHMARK_ADVANCED("单线程 co_spawn 10k 并执行")(Catch::Benchmark::Chronometer meter)
    {
        xin::async::IOContext context;

        meter.measure([&] {
            for (int i = 0; i < 10000; ++i)
                xin::async::co_spawn(context, []() -> xin::async::Task<> { co_return; }());
            context.run();
        });
    };

    // 测试 2：极致跨线程投递性能
    BENCHMARK_ADVANCED("跨线程仅 post 10k (测试 MPSC 队列吞吐)")(
        Catch::Benchmark::Chronometer meter)
    {
        xin::async::IOContext context;
        context.track(); // 保持工作线程不退出
        std::jthread worker([&] { context.run(); });

        meter.measure([&] {
            for (int i = 0; i < 10000; ++i)
                xin::async::co_spawn(context, []() -> xin::async::Task<> { co_return; }());
        });

        context.untrack();
        context.stop(); // 【关键修复】：敲响退出的铜锣，强行唤醒沉睡的内核！
        worker.join();
    };

    // 测试 3：跨线程全链路
    BENCHMARK_ADVANCED("跨线程 post 并等待 10k 执行完毕")(Catch::Benchmark::Chronometer meter)
    {
        xin::async::IOContext context;
        context.track();
        std::jthread worker([&] { context.run(); });

        std::atomic<int> counter;
        std::atomic_flag done;

        meter.measure([&] {
            counter.store(10000, std::memory_order_relaxed); // 恢复成 10000 的大考
            done.clear(std::memory_order_relaxed);

            for (int i = 0; i < 10000; ++i) // 恢复成 10000
                xin::async::co_spawn(context, benchmark3_worker(counter, done));

            done.wait(false, std::memory_order_acquire);
        });

        context.untrack();
        context.stop(); // 【关键修复】：敲响退出的铜锣，强行唤醒沉睡的内核！
        worker.join();
    };
}