module;

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

export module xin.utility.log;

import std;

export namespace xin {

/// @brief 表示日志等级。
enum class LogLevel : std::uint8_t { Trace, Debug, Info, Warning, Error, Critical };

/// @brief 日志记录器抽象基类。
class Logger {
public:
    Logger() = default;

    Logger(const Logger&) = delete;
    auto operator=(const Logger&) -> Logger& = delete;

    Logger(Logger&&) = default;
    auto operator=(Logger&&) -> Logger& = default;

    virtual ~Logger() = default;

    template<typename... Args>
    void trace(std::format_string<Args...> fmt, Args&&... args)
    {
        log(LogLevel::Trace, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void debug(std::format_string<Args...> fmt, Args&&... args)
    {
        log(LogLevel::Debug, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args)
    {
        log(LogLevel::Info, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void warning(std::format_string<Args...> fmt, Args&&... args)
    {
        log(LogLevel::Warning, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void error(std::format_string<Args...> fmt, Args&&... args)
    {
        log(LogLevel::Error, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void critical(std::format_string<Args...> fmt, Args&&... args)
    {
        log(LogLevel::Critical, std::format(fmt, std::forward<Args>(args)...));
    }

    void set_level(const LogLevel level) noexcept
    {
        level_ = level;
        set_level_impl(level);
    }

    [[nodiscard]]
    constexpr auto level() const noexcept -> LogLevel
    {
        return level_;
    }

    void set_pattern(const std::string_view pattern)
    {
        set_pattern_impl(pattern);
    }

private:
    LogLevel level_ = LogLevel::Info;

    virtual void log(LogLevel level, std::string_view message) = 0;

    virtual void set_level_impl(LogLevel level) = 0;

    virtual void set_pattern_impl(std::string_view pattern) = 0;
};

/// @brief 全局日志门面。
struct log {
    log() = delete;

    static void set_level(const LogLevel level)
    {
        logger().set_level(level);
    }

    static auto level() noexcept -> LogLevel
    {
        return logger().level();
    }

    static void set_pattern(const std::string_view pattern)
    {
        logger().set_pattern(pattern);
    }

    static void set_default_logger(std::unique_ptr<Logger> logger)
    {
        default_logger = std::move(logger);
    }

    template<typename... Args>
    static void trace(std::format_string<Args...> fmt, Args&&... args)
    {
        logger().trace(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void debug(std::format_string<Args...> fmt, Args&&... args)
    {
        logger().debug(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void info(std::format_string<Args...> fmt, Args&&... args)
    {
        logger().info(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void warning(std::format_string<Args...> fmt, Args&&... args)
    {
        logger().warning(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error(std::format_string<Args...> fmt, Args&&... args)
    {
        logger().error(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void critical(std::format_string<Args...> fmt, Args&&... args)
    {
        logger().critical(fmt, std::forward<Args>(args)...);
    }

private:
    static std::unique_ptr<Logger> default_logger;

    static auto logger() -> Logger&
    {
        return *default_logger;
    }
};

} // namespace xin

module :private;

namespace xin {

/// @brief 基于 spdlog 的默认日志实现。
class Spdlog : public Logger {
public:
    Spdlog()
    {
        const auto logger = spdlog::stdout_color_mt("xin");
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] %^[%l]%$ %v");
        spdlog::set_default_logger(logger);

        spdlog::set_level(spdlog::level::info);
    }

    virtual ~Spdlog() = default;

private:
    void log(const LogLevel level, const std::string_view message) override
    {
        switch (level) {
        case LogLevel::Trace:
            spdlog::trace(message);
            break;
        case LogLevel::Debug:
            spdlog::debug(message);
            break;
        case LogLevel::Info:
            spdlog::info(message);
            break;
        case LogLevel::Warning:
            spdlog::warn(message);
            break;
        case LogLevel::Error:
            spdlog::error(message);
            break;
        case LogLevel::Critical:
            spdlog::critical(message);
            break;
        }
    }

    void set_level_impl(const LogLevel level) noexcept override
    {
        switch (level) {
        case LogLevel::Trace:
            spdlog::set_level(spdlog::level::trace);
            break;
        case LogLevel::Debug:
            spdlog::set_level(spdlog::level::debug);
            break;
        case LogLevel::Info:
            spdlog::set_level(spdlog::level::info);
            break;
        case LogLevel::Warning:
            spdlog::set_level(spdlog::level::warn);
            break;
        case LogLevel::Error:
            spdlog::set_level(spdlog::level::err);
            break;
        case LogLevel::Critical:
            spdlog::set_level(spdlog::level::critical);
            break;
        }
    }

    void set_pattern_impl(const std::string_view pattern) override
    {
        spdlog::set_pattern(std::string{ pattern });
    }
};

std::unique_ptr<Logger> log::default_logger = std::make_unique<Spdlog>();

} // namespace xin
