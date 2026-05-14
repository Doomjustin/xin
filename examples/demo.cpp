import std;

import xin;


using namespace std::chrono_literals;
using namespace xin;

auto demo() -> async::Task<>
{
    log::info("Hello, async world!");
    co_await async::sleep_for(10s);
    log::info("Goodbye, async world!");
}

auto stop_then(std::stop_source stop_source) -> async::Task<>
{
    co_await async::co_spawn(demo());

    log::info("This task will be stopped in 3 seconds.");
    co_await async::sleep_for(3s);
    stop_source.request_stop();
    log::info("Stop requested.");
}

int main(int argc, char* argv[])
{
    std::stop_source stop_source;
    auto token = stop_source.get_token();
    async::run(4, token, stop_then, stop_source);
    return 0;
}