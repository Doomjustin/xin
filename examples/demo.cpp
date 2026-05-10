import std;

import xin;


auto demo() -> xin::async::Task<>
{
    using namespace std::chrono_literals;

    xin::log::info("Hello, {}!", "world");
    co_await xin::async::sleep_for(1s);

    xin::log::info("awaked after 1 second");
}

int main(int argc, char* argv[])
{
    auto& context = xin::async::this_coroutine::context();

    xin::async::co_spawn(context, demo());
    xin::async::co_spawn(context, demo());

    context.run();

    xin::log::info("All tasks completed. Exiting.");
    return 0;
}