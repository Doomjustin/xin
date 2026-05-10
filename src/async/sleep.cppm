module;

#include <liburing.h>

export module xin.async.sleep;

import std;

import xin.async.io_context;
import xin.async.single_shot_operation;
import xin.async.this_coroutine;
import xin.utility;


export namespace xin::async {

/// @brief 基于 io_uring timeout 的单次 sleep awaiter。
///
/// 当 timeout 到期时，kernel 返回 `-ETIME`，该值在这里视为正常完成。
class TimerAwaier : public SingleShotOperation<TimerAwaier, void> {
public:
    /// @brief 按 duration 构造 sleep awaiter。
    /// @param[in] context 绑定的 IOContext。
    /// @param[in] duration 相对等待时长；负值会被钳制为 0。
    template<chrono_duration Duration>
    TimerAwaier(context_type& context, Duration duration)
      : SingleShotOperation<TimerAwaier, void>{ context }
    {
        if (duration < Duration::zero())
            timeout_ = { .tv_sec = 0, .tv_nsec = 0 };
        else
            timeout_ = cast_time(duration);
    }

    /// @brief 按绝对时间点构造 sleep awaiter。
    /// @param[in] context 绑定的 IOContext。
    /// @param[in] timepoint 目标唤醒时间点；若已过期则退化为 0 超时。
    template<typename Clock, typename Duration>
    TimerAwaier(context_type& context, std::chrono::time_point<Clock, Duration> timepoint)
      : SingleShotOperation<TimerAwaier, void>{ context }
    {
        auto now = Clock::now();

        if (now >= timepoint)
            timeout_ = { .tv_sec = 0, .tv_nsec = 0 };
        else
            timeout_ = cast_time(timepoint - now);
    }

    /// @brief 零超时时直接就绪，避免无意义提交 SQE。
    auto await_ready() const noexcept -> bool
    {
        return timeout_.tv_sec == 0 && timeout_.tv_nsec == 0;
    }

    /// @brief 准备 timeout SQE。
    void prepare(::io_uring_sqe* sqe) noexcept
    {
        ::io_uring_prep_timeout(sqe, &timeout_, 0, 0);
    }

    /// @brief 解析完成结果。
    /// @return 成功时返回空 expected；失败时返回 system error。
    auto await_resume() noexcept -> std::expected<void, std::error_code>
    {
        if (error_code_ == ETIME || error_code_ == 0)
            return {};

        return unexpected_system_error(error_code_);
    }

private:
    struct __kernel_timespec timeout_{};

    /// @brief 将 chrono duration 转换为 kernel timespec。
    auto cast_time(chrono_duration auto duration) -> __kernel_timespec
    {
        using namespace std::chrono;
        auto ns = duration_cast<nanoseconds>(duration).count();
        return { ns / 1'000'000'000, ns % 1'000'000'000 };
    }
};

/// @brief 在指定 context 上按相对时长休眠。
template<chrono_duration Duration>
auto sleep_for(IOContext& context, Duration duration) -> TimerAwaier
{
    return TimerAwaier{ context, duration };
}

/// @brief 在当前绑定 context 上按相对时长休眠。
template<chrono_duration Duration>
auto sleep_for(Duration duration) -> TimerAwaier
{
    return sleep_for(this_coroutine::context(), duration);
}

/// @brief 在指定 context 上休眠到绝对时间点。
template<typename Clock, typename Duration>
auto sleep_until(IOContext& context, std::chrono::time_point<Clock, Duration> timepoint)
    -> TimerAwaier
{
    return TimerAwaier{ context, timepoint };
}

/// @brief 在当前绑定 context 上休眠到绝对时间点。
template<typename Clock, typename Duration>
auto sleep_until(std::chrono::time_point<Clock, Duration> timepoint) -> TimerAwaier
{
    return sleep_until(this_coroutine::context(), timepoint);
}

} // namespace xin::async