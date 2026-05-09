#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.as_string;


namespace {

constexpr auto expected_text = std::string_view{ "xin" };
constexpr auto expected_size = expected_text.size();
constexpr auto zero_size = std::size_t{ 0 };

constexpr auto char_x = 'x';
constexpr auto char_i = 'i';
constexpr auto char_n = 'n';
constexpr auto char_x_upper = 'X';

} // namespace

TEST_CASE("xin::as_string supports const byte span", "[utility][as_string]")
{
    SECTION("returns correct view for const span")
    {
        const std::array<std::byte, expected_size> bytes{
            static_cast<std::byte>(char_x),
            static_cast<std::byte>(char_i),
            static_cast<std::byte>(char_n),
        };

        const std::span<const std::byte> data{ bytes };

        const auto text = xin::as_string(data);

        REQUIRE(text == expected_text);
        REQUIRE(text.size() == expected_size);
    }
}

TEST_CASE("xin::as_string supports mutable byte span", "[utility][as_string]")
{
    SECTION("returns correct view for mutable span")
    {
        std::array<std::byte, expected_size> bytes{
            static_cast<std::byte>(char_x),
            static_cast<std::byte>(char_i),
            static_cast<std::byte>(char_n),
        };

        std::span<std::byte> data{ bytes };

        const auto text = xin::as_string(data);

        REQUIRE(text == expected_text);
        REQUIRE(text.size() == expected_size);
    }

    SECTION("returned string_view reflects underlying data changes")
    {
        std::array<std::byte, expected_size> bytes{
            static_cast<std::byte>(char_x),
            static_cast<std::byte>(char_i),
            static_cast<std::byte>(char_n),
        };
        std::span<std::byte> data{ bytes };

        const auto text = xin::as_string(data);
        bytes.front() = static_cast<std::byte>(char_x_upper);

        REQUIRE(text.front() == char_x_upper);
    }
}

TEST_CASE("xin::as_string handles empty spans", "[utility][as_string]")
{
    SECTION("const span empty case")
    {
        const std::array<std::byte, zero_size> bytes{};
        const std::span<const std::byte> data{ bytes };

        const auto text = xin::as_string(data);

        REQUIRE(text.empty());
        REQUIRE(text.size() == zero_size);
    }

    SECTION("mutable span empty case")
    {
        std::array<std::byte, zero_size> bytes{};
        std::span<std::byte> data{ bytes };

        const auto text = xin::as_string(data);

        REQUIRE(text.empty());
        REQUIRE(text.size() == zero_size);
    }
}