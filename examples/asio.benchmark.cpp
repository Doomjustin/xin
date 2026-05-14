#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

// 引入 Boost.Asio 核心及协程支持
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>

import std;

namespace asio = boost::asio;

// 对标你的 benchmark3_worker
asio::awaitable<void> asio_benchmark3_worker(std::atomic<int>& c, std::atomic_flag& d)
{
    if (c.fetch_sub(1, std::memory_order_relaxed) == 1) {
        d.test_and_set(std::memory_order_release);
        d.notify_one();
    }
    co_return;
}

TEST_CASE("Boost.Asio 吞吐量基准测试", "[benchmark][asio]")
{
    // 测试 1：单线程全链路
    BENCHMARK_ADVANCED("Asio 单线程 co_spawn 10k 并执行")(Catch::Benchmark::Chronometer meter)
    {
        // 注意：传入 1 是 Asio 的 Concurrency Hint。
        // 这会告诉 Asio "我是单线程的"，Asio 会在内部关掉所有 mutex 锁，这是最极致的单线程优化。
        asio::io_context context{ 1 };

        meter.measure([&] {
            for (int i = 0; i < 10000; ++i) {
                asio::co_spawn(
                    context, []() -> asio::awaitable<void> { co_return; }(), asio::detached);
            }
            context.run();
        });
    };

    // 测试 2：极致跨线程投递性能
    BENCHMARK_ADVANCED("Asio 跨线程仅 post 10k")(Catch::Benchmark::Chronometer meter)
    {
        // 跨线程调度，不能给 hint 1
        asio::io_context context;
        // 相当于你的 context.track()，防止 run() 提前退出
        auto work_guard = asio::make_work_guard(context);
        std::jthread worker([&] { context.run(); });

        meter.measure([&] {
            for (int i = 0; i < 10000; ++i) {
                asio::co_spawn(
                    context, []() -> asio::awaitable<void> { co_return; }(), asio::detached);
            }
        });

        // 相当于你的 context.untrack() 和 context.stop()
        work_guard.reset();
        worker.join();
    };

    // 测试 3：跨线程全链路
    BENCHMARK_ADVANCED("Asio 跨线程 post 并等待 10k 执行完毕")(Catch::Benchmark::Chronometer meter)
    {
        asio::io_context context;
        auto work_guard = asio::make_work_guard(context);
        std::jthread worker([&] { context.run(); });

        meter.measure([&] {
            std::atomic<int> counter{ 10000 };
            std::atomic_flag done{};

            for (int i = 0; i < 10000; ++i)
                asio::co_spawn(context, asio_benchmark3_worker(counter, done), asio::detached);

            done.wait(false, std::memory_order_acquire);
        });

        work_guard.reset();
        worker.join();
    };
}