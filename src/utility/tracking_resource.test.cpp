
#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.tracking_resource;


namespace {

constexpr std::size_t element_count = 32U;

} // namespace

TEST_CASE("xin::TrackingMemoryResource 显式分配与释放会更新已用字节",
          "[utility][tracking_resource]")
{
    xin::TrackingMemoryResource resource{ std::pmr::new_delete_resource() };
    std::pmr::polymorphic_allocator<std::byte> allocator{ &resource };

    REQUIRE(resource.used_memory() == 0U);

    auto* block = allocator.allocate(element_count);
    REQUIRE(resource.used_memory() == element_count);

    allocator.deallocate(block, element_count);
    REQUIRE(resource.used_memory() == 0U);
}

TEST_CASE("xin::TrackingMemoryResource 可跟踪 pmr 容器生命周期", "[utility][tracking_resource]")
{
    xin::TrackingMemoryResource resource{ std::pmr::new_delete_resource() };

    {
        std::pmr::vector<int> values{ &resource };
        values.resize(element_count);

        REQUIRE(resource.used_memory() >= element_count * sizeof(int));
    }

    REQUIRE(resource.used_memory() == 0U);
}

TEST_CASE("xin::TrackingMemoryResource 与上游资源地址相等性独立", "[utility][tracking_resource]")
{
    xin::TrackingMemoryResource left{ std::pmr::new_delete_resource() };
    xin::TrackingMemoryResource right{ std::pmr::new_delete_resource() };

    REQUIRE(left.is_equal(left));
    REQUIRE_FALSE(left.is_equal(right));
}
