#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.as_string;


namespace {

constexpr auto kExpectedText = std::string_view{ "xin" };
constexpr auto kExpectedSize = kExpectedText.size();
constexpr auto kZeroSize = std::size_t{ 0 };

constexpr auto kCharX = 'x';
constexpr auto kCharI = 'i';
constexpr auto kCharN = 'n';
constexpr auto kCharXUpper = 'X';

} // namespace

TEST_CASE("xin::as_string supports const byte span", "[utility][as_string]")
{
    SECTION("returns correct view for const span")
    {
        const std::array<std::byte, kExpectedSize> bytes{
            static_cast<std::byte>(kCharX),
            static_cast<std::byte>(kCharI),
            static_cast<std::byte>(kCharN),
        };

        const std::span<const std::byte> data{ bytes };

        const auto text = xin::as_string(data);

        REQUIRE(text == kExpectedText);
        REQUIRE(text.size() == kExpectedSize);
    }
}

TEST_CASE("xin::as_string supports mutable byte span", "[utility][as_string]")
{
    SECTION("returns correct view for mutable span")
    {
        std::array<std::byte, kExpectedSize> bytes{
            static_cast<std::byte>(kCharX),
            static_cast<std::byte>(kCharI),
            static_cast<std::byte>(kCharN),
        };

        std::span<std::byte> data{ bytes };

        const auto text = xin::as_string(data);

        REQUIRE(text == kExpectedText);
        REQUIRE(text.size() == kExpectedSize);
    }

    SECTION("returned string_view reflects underlying data changes")
    {
        std::array<std::byte, kExpectedSize> bytes{
            static_cast<std::byte>(kCharX),
            static_cast<std::byte>(kCharI),
            static_cast<std::byte>(kCharN),
        };
        std::span<std::byte> data{ bytes };

        const auto text = xin::as_string(data);
        bytes.front() = static_cast<std::byte>(kCharXUpper);

        REQUIRE(text.front() == kCharXUpper);
    }
}

TEST_CASE("xin::as_string handles empty spans", "[utility][as_string]")
{
    SECTION("const span empty case")
    {
        const std::array<std::byte, kZeroSize> bytes{};
        const std::span<const std::byte> data{ bytes };

        const auto text = xin::as_string(data);

        REQUIRE(text.empty());
        REQUIRE(text.size() == kZeroSize);
    }

    SECTION("mutable span empty case")
    {
        std::array<std::byte, kZeroSize> bytes{};
        std::span<std::byte> data{ bytes };

        const auto text = xin::as_string(data);

        REQUIRE(text.empty());
        REQUIRE(text.size() == kZeroSize);
    }
}