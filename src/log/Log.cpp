module;

#include <cstdint>
#include <memory>
#include <utility>

#include "fmt/base.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

export module Log;

namespace xin::log  {

export enum class LogLevel: std::uint8_t {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
    Off
};

export class Logger {
private:
    LogLevel level_ = LogLevel::Debug;
    std::shared_ptr<spdlog::logger> logger_;

public:
    explicit Logger(std::string_view name)
      : logger_{ open(name) }
    {}

    Logger(const Logger& other) = delete;
    Logger& operator=(const Logger& other) = delete;

    Logger(Logger&& other) noexcept = default;
    Logger& operator=(Logger&& other) noexcept = default;

    ~Logger() = default;

    static std::shared_ptr<Logger> instance()
    {
        static auto logger = std::make_shared<Logger>("xin");
        return logger;
    }

    template<typename... Args>
    void trace(fmt::format_string<Args...> fmt, Args&&... args) const noexcept
    {
        log(LogLevel::Trace, fmt::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void debug(fmt::format_string<Args...> fmt, Args&&... args) const noexcept
    {
        log(LogLevel::Debug, fmt::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void info(fmt::format_string<Args...> fmt, Args&&... args) const noexcept
    {
        log(LogLevel::Info, fmt::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void warning(fmt::format_string<Args...> fmt, Args&&... args) const noexcept
    {
        log(LogLevel::Warning, fmt::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void error(fmt::format_string<Args...> fmt, Args&&... args) const noexcept
    {
        log(LogLevel::Error, fmt::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void critical(fmt::format_string<Args...> fmt, Args&&... args) const noexcept
    {
        log(LogLevel::Critical, fmt::format(fmt, std::forward<Args>(args)...));
    }

    void level(LogLevel new_value) noexcept
    {
        level_ = new_value;
        logger_->set_level(cast(new_value));
    }

    consteval LogLevel level() const noexcept { return level_; }

private:
    void log(LogLevel level, std::string_view message) const noexcept
    {
        logger_->log(cast(level), message);
    }
    
    static constexpr spdlog::level::level_enum cast(LogLevel level) noexcept
    {
        switch (level) {
        using enum LogLevel;
        case Trace:
            return spdlog::level::level_enum::trace;
        case Debug:
            return spdlog::level::level_enum::debug;
        case Info:
            return spdlog::level::level_enum::info; 
        case Warning:
            return spdlog::level::level_enum::warn;
        case Error:
            return spdlog::level::level_enum::err;
        case Critical:
            return spdlog::level::level_enum::critical;
        case Off:
            return spdlog::level::level_enum::off;
        }

        // unreachable
        return spdlog::level::off;
    }

    static std::shared_ptr<spdlog::logger> open(std::string_view name)
    {
        const auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        const auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/xin.txt", true);
        const spdlog::sinks_init_list sink_list = { file_sink, console_sink };

        auto logger = std::make_shared<spdlog::logger>(std::string{ name }, sink_list.begin(), sink_list.end());
        logger->set_level(spdlog::level::level_enum::debug);

        return logger;
    }
};

} // namespace xin::log 