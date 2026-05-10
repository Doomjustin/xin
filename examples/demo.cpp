import std;

import xin;


auto demo() -> xin::async::Task<>
{
    xin::log::info("Hello, {}!", "world");

    co_return;
}

int main(int argc, char* argv[])
{
    auto& context = xin::async::this_coroutine::context();

    xin::async::co_spawn(context, demo());
    xin::async::co_spawn(demo());

    context.run();

    xin::log::info("All tasks completed. Exiting.");
    return 0;
}