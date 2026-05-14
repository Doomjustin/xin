#include <csignal>

#include <pthread.h>

import std;

import xin;


using namespace std::chrono_literals;
using namespace xin;

auto demo() -> async::Task<>
{
    log::info("Hello, async world!");
    auto res = co_await async::sleep_for(10s);
    if (!res) {
        if (res.error() == std::errc::operation_canceled) {
            log::info("Demo task 被取消了.");
            co_return;
        }
        else {
            log::error("Failed to sleep: {}", res.error());
            co_return;
        }
    }

    log::info("Goodbye, async world!");
}

auto shutdown_monitor(std::stop_source stop_source) -> async::Task<>
{
    auto& context = co_await async::this_coro::context;
    log::info("Shutdown monitor started, waiting for signals...");

    async::SignalSet signals{ async::signals::interrupt, async::signals::terminate };
    auto res = co_await signals.async_wait();

    stop_source.request_stop();
    log::info("Shutdown signal received, stopping IOContext...");
}

auto stop_then(std::stop_source stop_source) -> async::Task<>
{
    auto& context = co_await async::this_coro::context;
    auto token = co_await async::this_coro::stop_token;

    log::info("stop_then started in context, stop_token.stop_possible={}", token.stop_possible());

    co_await async::co_spawn(demo());

    log::info("This task will be stopped in 3 seconds.");

    // 无论是到期还是被取消，都继续往下走，观察取消传播效果。
    auto res = co_await async::sleep_for(3s);
    log::info("sleep_for(3s) returned: has_value={}, error={}", res.has_value(),
              res.has_value() ? "N/A" : std::string{ res.error().message() });

    stop_source.request_stop();
    log::info("Stop requested.");
}

int main(int argc, char* argv[])
{
    async::signals::block(async::signals::interrupt, async::signals::terminate);

    std::stop_source stop_source;
    std::stop_token stop_token = stop_source.get_token();

    std::vector<std::jthread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([stop_source, stop_token]() mutable {
            // 工作线程继承主线程的信号屏蔽，无需额外处理
            async::run(stop_token, stop_then, stop_source);
        });
    }

    async::run(shutdown_monitor, stop_source);

    return 0;
}