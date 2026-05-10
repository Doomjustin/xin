#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.format;

namespace {

struct format_as_type {
    int value;
};

auto format_as(const format_as_type& value) -> std::string
{
    return std::format("format_as({})", value.value);
}

struct precedence_type {
    int value;

    auto to_string() const -> std::string
    {
        return std::format("to_string({})", value);
    }
};

auto format_as(const precedence_type& value) -> std::string
{
    return std::format("format_as({})", value.value);
}

struct to_string_type {
    int value;

    auto to_string() const -> std::string
    {
        return std::format("to_string({})", value);
    }
};

struct to_repr_type {
    int value;

    auto to_repr() const -> std::string
    {
        return std::format("to_repr({})", value);
    }
};

enum class sample_enum {
    alpha,
    beta,
};

struct ostream_type {
    int value;
};

auto operator<<(std::ostream& stream, const ostream_type& value) -> std::ostream&
{
    return stream << std::format("ostream({})", value.value);
}

constexpr auto expected_format_as = std::string_view{ "format_as(7)" };
constexpr auto expected_precedence = std::string_view{ "format_as(9)" };
constexpr auto expected_to_string = std::string_view{ "to_string(11)" };
constexpr auto expected_to_repr = std::string_view{ "to_repr(13)" };
constexpr auto expected_enum = std::string_view{ "alpha" };
constexpr auto expected_ostream = std::string_view{ "ostream(17)" };

} // namespace

TEST_CASE("xin::format 支持 format_as", "[utility][format]")
{
    const auto text = std::format("{}", format_as_type{ 7 });

    REQUIRE(text == expected_format_as);
}

TEST_CASE("xin::format 优先使用 format_as 而不是 to_string", "[utility][format]")
{
    const auto text = std::format("{}", precedence_type{ 9 });

    REQUIRE(text == expected_precedence);
}

TEST_CASE("xin::format 支持 to_string", "[utility][format]")
{
    const auto text = std::format("{}", to_string_type{ 11 });

    REQUIRE(text == expected_to_string);
}

TEST_CASE("xin::format 支持 to_repr", "[utility][format]")
{
    const auto text = std::format("{}", to_repr_type{ 13 });

    REQUIRE(text == expected_to_repr);
}

TEST_CASE("xin::format 支持 enum 格式化", "[utility][format]")
{
    const auto text = std::format("{}", sample_enum::alpha);

    REQUIRE(text == expected_enum);
}

TEST_CASE("xin::format 支持 ostream 回退格式化", "[utility][format]")
{
    const auto text = std::format("{}", ostream_type{ 17 });

    REQUIRE(text == expected_ostream);
}

TEST_CASE("xin::format 将 std::error_code 格式化为 message", "[utility][format]")
{
    const auto error_code = std::make_error_code(std::errc::permission_denied);

    const auto text = std::format("{}", error_code);

    REQUIRE(text == error_code.message());
}
