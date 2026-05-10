#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.cast;

TEST_CASE("xin::to_uppercase 将 ASCII 字母转为大写", "[utility][cast]")
{
    REQUIRE(xin::to_uppercase("xin-123") == "XIN-123");
    REQUIRE(xin::to_uppercase("Already UPPER") == "ALREADY UPPER");
    REQUIRE(xin::to_uppercase("") == "");
}

TEST_CASE("xin::to_lowercase 将 ASCII 字母转为小写", "[utility][cast]")
{
    REQUIRE(xin::to_lowercase("XIN-123") == "xin-123");
    REQUIRE(xin::to_lowercase("Already lower") == "already lower");
    REQUIRE(xin::to_lowercase("") == "");
}

TEST_CASE("xin::numeric_cast 解析整数成功", "[utility][cast]")
{
    const auto value = xin::numeric_cast<int>("42");

    REQUIRE(value.has_value());
    REQUIRE(value.value() == 42);
}

TEST_CASE("xin::numeric_cast 对非法输入返回错误", "[utility][cast]")
{
    const auto with_tail = xin::numeric_cast<int>("42abc");
    REQUIRE_FALSE(with_tail.has_value());
    REQUIRE(with_tail.error() == std::make_error_code(std::errc::invalid_argument));

    const auto not_number = xin::numeric_cast<int>("abc");
    REQUIRE_FALSE(not_number.has_value());
    REQUIRE(not_number.error() == std::make_error_code(std::errc::invalid_argument));

    const auto out_of_range = xin::numeric_cast<int>("999999999999999999999");
    REQUIRE_FALSE(out_of_range.has_value());
    REQUIRE(out_of_range.error() == std::make_error_code(std::errc::result_out_of_range));
}

TEST_CASE("xin::string_cast 浮点转字符串后可回读", "[utility][cast]")
{
    const auto text = xin::string_cast(12.5);
    REQUIRE(text.has_value());

    const auto parsed = xin::numeric_cast<double>(*text);
    REQUIRE(parsed.has_value());
    REQUIRE(*parsed == 12.5);
}
