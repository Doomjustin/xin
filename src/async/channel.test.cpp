#include <catch2/catch_test_macros.hpp>

import std;

import xin.async;


using namespace std::chrono_literals;

TEST_CASE("Channel 1：基础测试：有缓冲的单线程收发", "[async][channel]")
{
    xin::async::IOContext context;
    xin::async::Channel<int> channel{ 2 };

    int received1 = 0;
    int received2 = 0;

    auto test_worker = [](xin::async::Channel<int>& ch, int& r1, int& r2) -> xin::async::Task<> {
        co_await ch.async_send(42);
        co_await ch.async_send(100);
        auto res1 = co_await ch.async_receive();
        auto res2 = co_await ch.async_receive();
        if (res1)
            r1 = *res1;
        if (res2)
            r2 = *res2;
    };

    xin::async::co_spawn(context, test_worker(channel, received1, received2));
    context.run();

    REQUIRE(received1 == 42);
    REQUIRE(received2 == 100);
}

TEST_CASE("Channel 2：并发测试：无缓冲（容量0）的跨线程 Ping-Pong", "[async][channel]")
{
    xin::async::IOContext context;
    xin::async::Channel<std::string> channel{ 0 };

    context.track();
    std::jthread worker([&] { context.run(); });

    std::string result;

    auto receiver = [](xin::async::Channel<std::string>& ch,
                       std::string& res) -> xin::async::Task<> {
        auto val = co_await ch.async_receive();
        if (val)
            res = *val;
    };

    auto sender = [](xin::async::Channel<std::string>& ch) -> xin::async::Task<> {
        co_await xin::async::sleep_for(10ms);
        co_await ch.async_send("Hello from sender!");
    };

    xin::async::co_spawn(context, receiver(channel, result));
    xin::async::co_spawn(context, sender(channel));

    context.untrack();
    context.stop();
    worker.join();

    REQUIRE(result == "Hello from sender!");
}

TEST_CASE("Channel 3：取消测试：消费者被 StopToken 安全打断", "[async][channel]")
{
    xin::async::IOContext context;
    xin::async::Channel<int> channel{ 0 };
    std::stop_source stop_source;

    bool is_canceled = false;

    auto receiver = [](xin::async::Channel<int>& ch, bool& canceled) -> xin::async::Task<> {
        auto res = co_await ch.async_receive();
        if (!res && res.error() == std::errc::operation_canceled)
            canceled = true;
    };

    auto canceler = [](std::stop_source src) -> xin::async::Task<> {
        co_await xin::async::sleep_for(10ms);
        src.request_stop();
    };

    xin::async::co_spawn(context, stop_source.get_token(), receiver(channel, is_canceled));
    xin::async::co_spawn(context, canceler(stop_source));

    context.run();

    REQUIRE(is_canceled == true);
}

TEST_CASE("Channel 4：取消测试：发送者被 StopToken 安全取消", "[async][channel]")
{
    xin::async::IOContext context;
    xin::async::Channel<int> channel{ 0 }; // 容量为 0
    std::stop_source stop_source;

    bool is_canceled = false;

    // 唯一的写者：因为容量为 0 且没有读者，它会死死挂起
    auto sender = [](xin::async::Channel<int>& ch, bool& canceled) -> xin::async::Task<> {
        auto res = co_await ch.async_send(42);
        if (!res && res.error() == std::errc::operation_canceled)
            canceled = true;
    };

    auto canceler = [](std::stop_source src) -> xin::async::Task<> {
        co_await xin::async::sleep_for(10ms);
        src.request_stop();
    };

    xin::async::co_spawn(context, stop_source.get_token(), sender(channel, is_canceled));
    xin::async::co_spawn(context, canceler(stop_source));

    context.run();

    REQUIRE(is_canceled == true);
    REQUIRE(channel.empty() == true);
}

TEST_CASE("Channel 5：压力测试：多写一读 (MPSC) 不丢数据", "[async][channel]")
{
    xin::async::IOContext context;
    xin::async::Channel<int> channel{ 10 };

    context.track();
    std::jthread worker([&] { context.run(); });

    const int NUM_MESSAGES = 50000;
    std::atomic<int> sum_received{ 0 };

    auto mpsc_receiver = [](xin::async::Channel<int>& ch, std::atomic<int>& sum,
                            int total) -> xin::async::Task<> {
        for (int i = 0; i < total; ++i) {
            auto res = co_await ch.async_receive();
            if (res)
                sum.fetch_add(1, std::memory_order_relaxed);
        }
    };

    auto mpsc_sender = [](xin::async::Channel<int>& ch) -> xin::async::Task<> {
        co_await ch.async_send(1);
    };

    xin::async::co_spawn(context, mpsc_receiver(channel, sum_received, NUM_MESSAGES));

    std::vector<std::jthread> producers;
    for (int t = 0; t < 5; ++t) {
        producers.emplace_back([&] {
            for (int i = 0; i < 10000; ++i)
                xin::async::co_spawn(context, mpsc_sender(channel));
        });
    }

    for (auto& p : producers)
        p.join();

    context.untrack();
    context.stop();
    worker.join();

    REQUIRE(sum_received.load() == NUM_MESSAGES);
}
TEST_CASE("Channel 6：多写多读 (MPMC) 乱序竞争与优雅关闭", "[async][channel]")
{
    xin::async::IOContext context;
    xin::async::Channel<int> channel{ 5 }; // 小缓冲，制造高频竞争

    context.track();
    std::jthread worker([&] { context.run(); });

    const int NUM_PRODUCERS = 4;
    const int NUM_CONSUMERS = 4;
    const int MSG_PER_PRODUCER = 10000;
    const int TOTAL_MSG = NUM_PRODUCERS * MSG_PER_PRODUCER;

    std::atomic<int> total_received{ 0 };
    std::atomic<int> total_sent{ 0 };

    auto mpmc_receiver = [](xin::async::Channel<int>& ch,
                            std::atomic<int>& sum) -> xin::async::Task<> {
        while (true) {
            auto res = co_await ch.async_receive();
            if (res)
                sum.fetch_add(1, std::memory_order_relaxed);
            else
                break; // 通道关闭，安全下班
        }
    };

    auto mpmc_sender = [](xin::async::Channel<int>& ch,
                          std::atomic<int>& sent) -> xin::async::Task<> {
        auto res = co_await ch.async_send(1);
        if (res)
            sent.fetch_add(1, std::memory_order_relaxed);
    };

    // 启动 4 个消费者
    for (int i = 0; i < NUM_CONSUMERS; ++i)
        xin::async::co_spawn(context, mpmc_receiver(channel, total_received));

    // 启动 4 个并发生产者线程来派发任务
    std::vector<std::jthread> producers;
    for (int t = 0; t < NUM_PRODUCERS; ++t) {
        producers.emplace_back([&] {
            for (int i = 0; i < MSG_PER_PRODUCER; ++i)
                xin::async::co_spawn(context, mpmc_sender(channel, total_sent));
        });
    }

    for (auto& p : producers)
        p.join();

    // 死等所有异步发送任务真正完成！
    while (total_sent.load(std::memory_order_relaxed) < TOTAL_MSG)
        std::this_thread::yield();

    // 所有数据都已确切地进入 Channel，现在可以安全关闭了
    channel.close();

    context.untrack();
    worker.join();

    REQUIRE(total_received.load() == TOTAL_MSG);
}

TEST_CASE("Channel 7：Move-Only 类型 (unique_ptr) 内存所有权测试", "[async][channel]")
{
    xin::async::IOContext context;
    xin::async::Channel<std::unique_ptr<int>> channel{ 1 };

    int result_val = 0;

    auto test_worker = [](xin::async::Channel<std::unique_ptr<int>>& ch,
                          int& res) -> xin::async::Task<> {
        auto ptr = std::make_unique<int>(999);
        // 必须使用 std::move，编译期保证所有权安全转移
        co_await ch.async_send(std::move(ptr));

        auto recv_ptr = co_await ch.async_receive();
        if (recv_ptr && *recv_ptr)
            res = **recv_ptr;
    };

    xin::async::co_spawn(context, test_worker(channel, result_val));
    context.run();

    REQUIRE(result_val == 999);
}