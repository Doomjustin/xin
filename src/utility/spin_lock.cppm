module;

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#endif

export module xin.utility.spin_lock;

import std;

namespace detail {

void cpu_relax() noexcept
{
#if defined(__x86_64__) || defined(__i386__)
    _mm_pause();
#elif defined(__aarch64__) || defined(__arm__)
    __asm__ volatile("yield" ::: "memory");
#else
    std::this_thread::yield();
#endif
}

} // namespace detail

export namespace xin {

/// @brief 轻量自旋锁，适合极短临界区的线程同步。
///
/// 该锁基于 `std::atomic_flag` 实现，优先短暂自旋，超过阈值后让出 CPU，
/// 以降低高竞争时的忙等成本。
///
/// ```cpp
/// xin::SpinLock lock;
/// std::lock_guard guard{ lock };
/// ```
class SpinLock {
public:
    /// @brief 阻塞直到成功获取锁。
    void lock() noexcept
    {
        for (int spin = 0; flag_.test_and_set(std::memory_order_acquire); ++spin)
            if (spin < 16)
                detail::cpu_relax();
            else
                std::this_thread::yield();
    }

    /// @brief 释放当前持有的锁。
    void unlock() noexcept
    {
        flag_.clear(std::memory_order_release);
    }

    /// @brief 尝试无阻塞获取锁。
    /// @return 获取成功返回 `true`，否则返回 `false`。
    [[nodiscard]]
    auto try_lock() noexcept -> bool
    {
        return !flag_.test_and_set(std::memory_order_acquire);
    }

private:
    std::atomic_flag flag_;
};

} // namespace xin