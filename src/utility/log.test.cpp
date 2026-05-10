#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.log;

namespace {

struct capturing_logger : xin::Logger {
    void log(xin::LogLevel level, std::string_view message) override
    {
        last_level = level;
        last_message = std::string{ message };
        ++log_calls;
    }

    void set_level_impl(xin::LogLevel level) override
    {
        current_level = level;
        ++set_level_calls;
    }

    void set_pattern_impl(std::string_view pattern) override
    {
        current_pattern = std::string{ pattern };
        ++set_pattern_calls;
    }

    xin::LogLevel current_level = xin::LogLevel::Info;
    xin::LogLevel last_level = xin::LogLevel::Info;
    std::string current_pattern;
    std::string last_message;
    std::size_t log_calls = 0;
    std::size_t set_level_calls = 0;
    std::size_t set_pattern_calls = 0;
};

constexpr auto pattern_value = std::string_view{ "[%l] %v" };
constexpr auto trace_text = std::string_view{ "trace {}" };
constexpr auto debug_text = std::string_view{ "debug {}" };
constexpr auto info_text = std::string_view{ "info {}" };
constexpr auto warning_text = std::string_view{ "warning {}" };
constexpr auto error_text = std::string_view{ "error {}" };
constexpr auto critical_text = std::string_view{ "critical {}" };

} // namespace

TEST_CASE("xin::log 转发 level 和 pattern 变更", "[utility][log]")
{
    auto logger = std::make_unique<capturing_logger>();
    auto* logger_ptr = logger.get();

    xin::log::set_default_logger(std::move(logger));

    SECTION("level 变更被转发")
    {
        xin::log::set_level(xin::LogLevel::Warning);

        REQUIRE(xin::log::level() == xin::LogLevel::Warning);
        REQUIRE(logger_ptr->current_level == xin::LogLevel::Warning);
        REQUIRE(logger_ptr->set_level_calls == 1U);
    }

    SECTION("pattern 变更被转发")
    {
        xin::log::set_pattern(pattern_value);

        REQUIRE(logger_ptr->current_pattern == pattern_value);
        REQUIRE(logger_ptr->set_pattern_calls == 1U);
    }
}

TEST_CASE("xin::log 分发格式化后的消息", "[utility][log]")
{
    auto logger = std::make_unique<capturing_logger>();
    auto* logger_ptr = logger.get();

    xin::log::set_default_logger(std::move(logger));

    xin::log::trace(trace_text, 1);
    REQUIRE(logger_ptr->last_level == xin::LogLevel::Trace);
    REQUIRE(logger_ptr->last_message == "trace 1");

    xin::log::debug(debug_text, 2);
    REQUIRE(logger_ptr->last_level == xin::LogLevel::Debug);
    REQUIRE(logger_ptr->last_message == "debug 2");

    xin::log::info(info_text, 3);
    REQUIRE(logger_ptr->last_level == xin::LogLevel::Info);
    REQUIRE(logger_ptr->last_message == "info 3");

    xin::log::warning(warning_text, 4);
    REQUIRE(logger_ptr->last_level == xin::LogLevel::Warning);
    REQUIRE(logger_ptr->last_message == "warning 4");

    xin::log::error(error_text, 5);
    REQUIRE(logger_ptr->last_level == xin::LogLevel::Error);
    REQUIRE(logger_ptr->last_message == "error 5");

    xin::log::critical(critical_text, 6);
    REQUIRE(logger_ptr->last_level == xin::LogLevel::Critical);
    REQUIRE(logger_ptr->last_message == "critical 6");
    REQUIRE(logger_ptr->log_calls == 6U);
}
