#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.lru_cache;

namespace {

using cache_type = xin::LRUCache<std::string, int>;

auto ordered_keys(const cache_type& cache) -> std::vector<std::string>
{
    std::vector<std::string> keys;
    keys.reserve(cache.size());

    for (const auto& [key, value] : cache) {
        static_cast<void>(value);
        keys.push_back(key);
    }

    return keys;
}

} // namespace

TEST_CASE("xin::LRUCache 插入元素并维护最近使用顺序", "[utility][lru_cache]")
{
    cache_type cache{ 3 };

    cache.put("a", 1);
    cache.put("b", 2);
    cache.put("c", 3);

    REQUIRE(cache.capacity() == 3U);
    REQUIRE(cache.size() == 3U);
    REQUIRE(ordered_keys(cache) == std::vector<std::string>{ "c", "b", "a" });
}

TEST_CASE("xin::LRUCache 命中 get 时返回引用并提升顺序", "[utility][lru_cache]")
{
    cache_type cache{ 3 };
    cache.put("a", 1);
    cache.put("b", 2);
    cache.put("c", 3);

    auto value = cache.get("a");

    REQUIRE(value.has_value());
    REQUIRE(value->get() == 1);

    value->get() = 11;

    REQUIRE(cache.get("a")->get() == 11);
    REQUIRE(ordered_keys(cache) == std::vector<std::string>{ "a", "c", "b" });
}

TEST_CASE("xin::LRUCache 未命中 get 返回空 optional", "[utility][lru_cache]")
{
    cache_type cache{ 2 };
    cache.put("a", 1);

    const auto value = cache.get("missing");

    REQUIRE_FALSE(value.has_value());
    REQUIRE(cache.size() == 1U);
}

TEST_CASE("xin::LRUCache 容量满时淘汰最久未使用元素", "[utility][lru_cache]")
{
    cache_type cache{ 2 };
    cache.put("a", 1);
    cache.put("b", 2);

    REQUIRE(cache.get("a").has_value());

    cache.put("c", 3);

    REQUIRE(cache.get("a").has_value());
    REQUIRE(cache.get("c").has_value());
    REQUIRE_FALSE(cache.get("b").has_value());
    REQUIRE(ordered_keys(cache) == std::vector<std::string>{ "c", "a" });
}

TEST_CASE("xin::LRUCache 更新已有键时覆盖值并提升顺序", "[utility][lru_cache]")
{
    cache_type cache{ 2 };
    cache.put("a", 1);
    cache.put("b", 2);

    cache.put("a", 42);

    REQUIRE(cache.size() == 2U);
    REQUIRE(cache.get("a")->get() == 42);
    REQUIRE(ordered_keys(cache) == std::vector<std::string>{ "a", "b" });
}

TEST_CASE("xin::LRUCache clear 清空所有元素", "[utility][lru_cache]")
{
    cache_type cache{ 2 };
    cache.put("a", 1);
    cache.put("b", 2);

    cache.clear();

    REQUIRE(cache.size() == 0U);
    REQUIRE(cache.begin() == cache.end());
    REQUIRE_FALSE(cache.get("a").has_value());
}
