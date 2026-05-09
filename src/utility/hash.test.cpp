#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.hash;


namespace {

constexpr auto text = std::string_view{ "xin" };
constexpr auto other_text = std::string_view{ "hash" };
constexpr auto empty_text = std::string_view{};
constexpr auto fnv_offset_basis = std::uint64_t{ 14695981039346656037ULL };
constexpr auto expected_xin_fnv1a = std::uint64_t{ 13816373064200854860ULL };

} // namespace

TEST_CASE("xin::hash 支持 std::hash 路径", "[utility][hash]")
{
    const auto expected = std::hash<std::string_view>{}(text);

    REQUIRE(xin::hash(xin::use_std_hash(text)) == expected);
}

TEST_CASE("xin::hash 支持 FNV-1a 路径", "[utility][hash]")
{
    REQUIRE(xin::hash(xin::use_fnv_1a(text)) == expected_xin_fnv1a);
    REQUIRE(xin::hash(xin::use_fnv_1a(text)) != xin::hash(xin::use_fnv_1a(other_text)));
}

TEST_CASE("xin::hash 对空字符串返回 FNV offset basis", "[utility][hash]")
{
    REQUIRE(xin::hash(xin::use_fnv_1a(empty_text)) == fnv_offset_basis);
}

TEST_CASE("xin::StringHash 与 xin::PmrStringHash 使用相同的 FNV-1a 结果", "[utility][hash]")
{
    const auto expected = static_cast<std::size_t>(xin::hash(xin::use_fnv_1a(text)));

    REQUIRE(xin::StringHash{}(text) == expected);
    REQUIRE(xin::PmrStringHash{}(text) == expected);
}

TEST_CASE("xin::StringHash 可用于透明查找场景", "[utility][hash]")
{
    std::unordered_set<std::string, xin::StringHash, std::equal_to<>> values{ "xin", "hash" };

    REQUIRE(values.contains(text));
    REQUIRE(values.contains(other_text));
    REQUIRE_FALSE(values.contains(std::string_view{ "missing" }));
}
