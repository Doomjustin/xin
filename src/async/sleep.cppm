export module xin.async.sleep;

import std;

import xin.async.io_context;
import xin.async.this_coro;
import xin.async.timer_awaiter;
import xin.utility;


export namespace xin::async {

template<chrono_duration Duration>
auto sleep_for(Duration duration) -> TimerAwaiter
{
    return TimerAwaiter{ duration };
}

template<typename Clock, typename Duration>
auto sleep_until(std::chrono::time_point<Clock, Duration> timepoint) -> TimerAwaiter
{
    return TimerAwaiter{ timepoint };
}

} // namespace xin::async