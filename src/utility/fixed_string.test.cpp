#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.fixed_string;


namespace {

constexpr auto expected_text = std::string_view{ "xin" };
constexpr auto expected_size = expected_text.size();

} // namespace

TEST_CASE("xin::FixedString 支持从字符串字面量推导长度", "[utility][fixed_string]")
{
    constexpr xin::FixedString text{ "xin" };

    STATIC_REQUIRE(std::same_as<decltype(text), const xin::FixedString<4>>);
    REQUIRE(text.capacity == 4U);
    REQUIRE(text.view() == expected_text);
}

TEST_CASE("xin::FixedString 保留结尾空字符并暴露底层存储", "[utility][fixed_string]")
{
    constexpr xin::FixedString text{ "xin" };

    REQUIRE(text.data.size() == 4U);
    REQUIRE(text.data[0] == 'x');
    REQUIRE(text.data[1] == 'i');
    REQUIRE(text.data[2] == 'n');
    REQUIRE(text.data[3] == '\0');
}

TEST_CASE("xin::FixedString 的 view 不包含结尾空字符", "[utility][fixed_string]")
{
    constexpr xin::FixedString text{ "xin" };
    const auto view = text.view();

    REQUIRE(view == expected_text);
    REQUIRE(view.size() == expected_size);
    REQUIRE(view.data() == text.data.data());
}

TEST_CASE("xin::FixedString 支持默认比较", "[utility][fixed_string]")
{
    constexpr xin::FixedString first{ "alpha" };
    constexpr xin::FixedString second{ "alpha" };
    constexpr xin::FixedString third{ "bravo" };

    REQUIRE(first == second);
    REQUIRE(first < third);
    REQUIRE(third > second);
}
