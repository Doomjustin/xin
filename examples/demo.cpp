import std;

import xin;


using namespace std::chrono_literals;

auto demo(std::chrono::seconds duration) -> xin::async::Task<>
{
    xin::log::info("Hello, {}!", "world");

    co_await xin::async::timeout(xin::async::sleep_for(5s), duration);

    xin::log::info("awaked after {} second", duration.count());
}

int main(int argc, char* argv[])
{
    auto& context = xin::async::this_coroutine::context();

    xin::async::co_spawn(context, demo(3s));
    xin::async::co_spawn(context, demo(1s));

    context.run();

    xin::log::info("All tasks completed. Exiting.");
    return 0;
}