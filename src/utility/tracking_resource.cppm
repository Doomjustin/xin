module;

#include <gsl/gsl>

export module xin.utility.tracking_resource;

import std;

export namespace xin {

/// @brief 可跟踪当前已分配字节数的 `std::pmr::memory_resource` 包装器。
///
/// 该资源把实际分配与释放委托给上游 `memory_resource`，并在每次分配/释放时
/// 更新内部计数。`used_memory()` 返回当前净占用字节数。
///
/// ```cpp
/// xin::TrackingMemoryResource resource{ std::pmr::new_delete_resource() };
/// std::pmr::vector<int> values{ &resource };
/// values.resize(8);
/// ```
class TrackingMemoryResource : public std::pmr::memory_resource {
public:
    /// @brief 构造跟踪资源。
    /// @param[in] upstream 实际执行分配/释放的上游资源。
    explicit TrackingMemoryResource(gsl::not_null<std::pmr::memory_resource*> upstream)
      : upstream_{ upstream }
    {}

    /// @brief 返回当前净占用内存字节数。
    /// @return 当前已分配减已释放后的字节数。
    [[nodiscard]]
    auto used_memory() const noexcept -> std::size_t
    {
        return total_allocated_.load(std::memory_order_relaxed);
    }

private:
    std::pmr::memory_resource* upstream_;
    std::atomic<std::size_t> total_allocated_{ 0 };

    auto do_allocate(std::size_t bytes, std::size_t alignment) -> void* override
    {
        void* ptr = upstream_->allocate(bytes, alignment);
        total_allocated_.fetch_add(bytes, std::memory_order_relaxed);
        return ptr;
    }

    void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override
    {
        upstream_->deallocate(ptr, bytes, alignment);
        total_allocated_.fetch_sub(bytes, std::memory_order_relaxed);
    }

    [[nodiscard]]
    constexpr auto do_is_equal(const std::pmr::memory_resource& other) const noexcept
        -> bool override
    {
        return this == &other;
    }
};

} // namespace xin