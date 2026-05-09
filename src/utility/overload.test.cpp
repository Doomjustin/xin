#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.overload;


namespace {

using variant_type = std::variant<int, std::string, double>;

} // namespace

TEST_CASE("xin::Overload 可用于 std::visit 的多类型分发", "[utility][overload]")
{
    const auto visitor = xin::Overload{
        [](int value) { return std::format("int:{}", value); },
        [](const std::string& text) { return std::format("string:{}", text); },
        [](double value) { return std::format("double:{:.1f}", value); },
    };

    REQUIRE(std::visit(visitor, variant_type{ 7 }) == "int:7");
    REQUIRE(std::visit(visitor, variant_type{ std::string{ "xin" } }) == "string:xin");
    REQUIRE(std::visit(visitor, variant_type{ 1.5 }) == "double:1.5");
}

TEST_CASE("xin::Overload 保留不同参数签名的重载能力", "[utility][overload]")
{
    auto overload = xin::Overload{
        [](int value) { return value + 1; },
        [](std::string_view text) { return text.size(); },
        [](const auto& value) { return value ? 1 : 0; },
    };

    REQUIRE(overload(4) == 5);
    REQUIRE(overload(std::string_view{ "xin" }) == 3U);
    REQUIRE(overload(true) == 1);
}

TEST_CASE("xin::Overload 支持 mutable lambda 共享状态", "[utility][overload]")
{
    auto overload = xin::Overload{
        [count = 0](int value) mutable {
            count += value;
            return count;
        },
        [](std::string_view text) { return text.size(); },
    };

    REQUIRE(overload(2) == 2);
    REQUIRE(overload(3) == 5);
    REQUIRE(overload(std::string_view{ "ab" }) == 2U);
}