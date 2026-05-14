#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

import std;

import xin.async;


// --- 纯粹的协程任务定义 ---
auto bench_receiver(xin::async::Channel<int>& ch, int count, std::atomic_flag* done = nullptr)
    -> xin::async::Task<>
{
    for (int i = 0; i < count; ++i)
        co_await ch.async_receive();
    if (done) {
        done->test_and_set(std::memory_order_release);
        done->notify_one();
    }
}

auto bench_sender(xin::async::Channel<int>& ch, int count) -> xin::async::Task<>
{
    for (int i = 0; i < count; ++i)
        co_await ch.async_send(1);
}

// --- 基准测试套件 ---
TEST_CASE("Channel 吞吐量极致基准测试", "[benchmark][channel]")
{
    // 测试 1：单线程纯内存极速 (完全没有内核介入，测试 Awaiter 开销)
    BENCHMARK_ADVANCED("单线程缓冲 Channel 收发 10k 次")(Catch::Benchmark::Chronometer meter)
    {
        xin::async::IOContext context;
        xin::async::Channel<int> channel{ 10000 }; // 足够大的缓冲

        meter.measure([&] {
            // 先发 10k，再收 10k，全在单线程且无需挂起
            xin::async::co_spawn(context, bench_sender(channel, 10000));
            xin::async::co_spawn(context, bench_receiver(channel, 10000));
            context.run();
        });
    };

    // 测试 2：跨线程 1V1 Ping-Pong (无缓冲，强制跨线程唤醒)
    BENCHMARK_ADVANCED("跨线程无缓冲 Ping-Pong 10k 次")(Catch::Benchmark::Chronometer meter)
    {
        xin::async::IOContext context;
        xin::async::Channel<int> channel{ 0 }; // 0容量，强制交接

        context.track();
        std::jthread worker([&] { context.run(); });

        meter.measure([&] {
            std::atomic_flag done{};

            // 读者在主事件循环运行
            xin::async::co_spawn(context, bench_receiver(channel, 10000, &done));
            // 写者也在主事件循环，交替唤醒
            xin::async::co_spawn(context, bench_sender(channel, 10000));

            done.wait(false, std::memory_order_acquire);
        });

        context.untrack();
        context.stop();
        worker.join();
    };

    // 测试 3：地狱级 MPSC 压测 (4个独立系统线程 -> 1个消费线程)
    BENCHMARK_ADVANCED("跨线程 MPSC (4写1读) 共 40k 次")(Catch::Benchmark::Chronometer meter)
    {
        xin::async::IOContext context;
        xin::async::Channel<int> channel{ 1024 }; // 适度缓冲

        context.track();
        std::jthread worker([&] { context.run(); });

        meter.measure([&] {
            std::atomic_flag done{};

            // 1 个读者，要收 40000 次
            xin::async::co_spawn(context, bench_receiver(channel, 40000, &done));

            // 4 个真正的 OS 线程，疯狂并发写！
            std::vector<std::jthread> producers;
            for (int t = 0; t < 4; ++t) {
                producers.emplace_back([&] {
                    // 每个线程把自己的写者协程投递到 context 中运行
                    xin::async::co_spawn(context, bench_sender(channel, 10000));
                });
            }

            for (auto& p : producers)
                p.join();
            done.wait(false, std::memory_order_acquire);
        });

        context.untrack();
        context.stop();
        worker.join();
    };
}