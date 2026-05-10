import std;

import xin;


using namespace std::chrono_literals;

auto stop(std::stop_source& resource) -> xin::async::Task<>
{
    co_await xin::async::sleep_for(2s);
    xin::log::info("Stop requested. Stopping...");
    resource.request_stop();
}

auto demo(std::stop_token stop_token) -> xin::async::Task<>
{
    xin::log::info("Hello, {}!", "world");

    co_await xin::async::stop_then(xin::async::sleep_for(5s), stop_token);

    xin::log::info("stopped");
}

int main(int argc, char* argv[])
{
    std::stop_source stop_source;
    xin::async::co_spawn(stop(stop_source));

    xin::async::run(3, stop_source, demo);

    return 0;
}