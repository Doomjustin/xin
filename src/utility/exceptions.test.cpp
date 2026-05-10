#include <cerrno>

#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.exceptions;

namespace {

constexpr auto explicit_message = std::string_view{ "explicit error value" };
constexpr auto implicit_message = std::string_view{ "implicit errno value" };
constexpr auto format_suffix = std::string_view{ "context" };
constexpr int explicit_error_value = 22;
constexpr std::errc errc_value = std::errc::permission_denied;

} // namespace

TEST_CASE("throw_system_error 使用显式错误码时抛出 std::system_error", "[utility][exceptions]")
{
    SECTION("错误码、category 和消息均被保留")
    {
        try {
            xin::throw_system_error(explicit_error_value, "{}: {}", explicit_message,
                                    format_suffix);

            FAIL("expected std::system_error");
        }
        catch (const std::system_error& ex) {
            CHECK(ex.code().value() == explicit_error_value);
            CHECK(ex.code().category() == std::generic_category());

            const auto what_text = std::string_view{ ex.what() };
            CHECK(what_text.contains(explicit_message));
            CHECK(what_text.contains(format_suffix));
        }
    }
}

TEST_CASE("throw_system_error 使用 errno 时抛出 std::system_error", "[utility][exceptions]")
{
    SECTION("使用当前 errno 作为错误值")
    {
        errno = EINVAL;

        try {
            xin::throw_system_error("{}: {}", implicit_message, format_suffix);

            FAIL("expected std::system_error");
        }
        catch (const std::system_error& ex) {
            CHECK(ex.code().value() == EINVAL);
            CHECK(ex.code().category() == std::generic_category());

            const auto what_text = std::string_view{ ex.what() };
            CHECK(what_text.contains(implicit_message));
            CHECK(what_text.contains(format_suffix));
        }
    }
}

TEST_CASE("unexpected_system_error 返回 unexpected<error_code>", "[utility][exceptions]")
{
    SECTION("无参数时使用 errno 和 system_category")
    {
        errno = ENOENT;

        const auto result = xin::unexpected_system_error();

        CHECK(result.error().value() == ENOENT);
        CHECK(result.error().category() == std::system_category());
    }

    SECTION("传入 errc 时使用 make_error_code(errc)")
    {
        const auto result = xin::unexpected_system_error(errc_value);
        const auto expected = std::make_error_code(errc_value);

        CHECK(result.error() == expected);
        CHECK(result.error().category() == std::generic_category());
    }
}
