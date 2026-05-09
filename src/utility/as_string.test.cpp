#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.as_string;


namespace {

constexpr auto expected_text = std::string_view{ "xin" };
constexpr auto expected_size = expected_text.size();

constexpr auto to_byte(char c) -> std::byte
{
    return static_cast<std::byte>(c);
}

} // namespace

TEST_CASE("xin::as_string 支持 const byte span", "[utility][as_string]")
{
    SECTION("const span 返回正确视图")
    {
        const std::array<std::byte, expected_size> bytes{
            to_byte('x'),
            to_byte('i'),
            to_byte('n'),
        };

        const std::span<const std::byte> data{ bytes };

        const auto text = xin::as_string(data);

        REQUIRE(text == expected_text);
        REQUIRE(text.size() == expected_size);
    }
}

TEST_CASE("xin::as_string 支持 mutable byte span", "[utility][as_string]")
{
    SECTION("mutable byte span 返回正确视图")
    {
        std::array<std::byte, expected_size> bytes{
            to_byte('x'),
            to_byte('i'),
            to_byte('n'),
        };

        std::span<std::byte> data{ bytes };

        const auto text = xin::as_string(data);

        REQUIRE(text == expected_text);
        REQUIRE(text.size() == expected_size);
    }

    SECTION("mutable byte span 返回的 string_view 反映底层数据变化")
    {
        std::array<std::byte, expected_size> bytes{
            to_byte('x'),
            to_byte('i'),
            to_byte('n'),
        };
        std::span<std::byte> data{ bytes };

        const auto text = xin::as_string(data);
        bytes.front() = to_byte('X');

        REQUIRE(text.front() == 'X');
    }
}

TEST_CASE("xin::as_string 处理空 span", "[utility][as_string]")
{
    SECTION("const span 空输入场景")
    {
        const std::array<std::byte, 0> bytes{};
        const std::span<const std::byte> data{ bytes };

        const auto text = xin::as_string(data);

        REQUIRE(text.empty());
        REQUIRE(text.size() == 0U);
    }

    SECTION("mutable span 空输入场景")
    {
        std::array<std::byte, 0> bytes{};
        std::span<std::byte> data{ bytes };

        const auto text = xin::as_string(data);

        REQUIRE(text.empty());
        REQUIRE(text.size() == 0U);
    }
}