module;

#include <cerrno>

export module xin.utility.exceptions;

import std;

export namespace xin {

/// @brief 按指定错误码抛出系统错误。
/// @tparam Args 格式化参数类型。
/// @param[in] error 错误码。
/// @param[in] fmt 格式字符串。
/// @param[in] args 格式化参数。
/// ```cpp
/// xin::throw_system_error(ENOENT, "open {} failed", path);
/// ```
template <typename... Args>
void throw_system_error(int error, std::format_string<Args...> fmt, Args&&... args)
{
    throw std::system_error{
        error,
        std::generic_category(),
        std::format(fmt, std::forward<Args>(args)...),
    };
}

/// @brief 使用当前 errno 抛出系统错误。
/// @tparam Args 格式化参数类型。
/// @param[in] fmt 格式字符串。
/// @param[in] args 格式化参数。
template <typename... Args>
void throw_system_error(std::format_string<Args...> fmt, Args&&... args)
{
    throw std::system_error{
        errno,
        std::generic_category(),
        std::format(fmt, std::forward<Args>(args)...),
    };
}

/// @brief 使用当前 errno 构造 unexpected<error_code>。
/// @return 包含当前 errno 的 unexpected<error_code>。
auto unexpected_system_error() -> std::unexpected<std::error_code>
{
    return std::unexpected{ std::error_code{ errno, std::system_category() } };
}

/// @brief 将 std::errc 转换为 unexpected<error_code>。
/// @param[in] error 标准错误枚举值。
/// @return 对应的 unexpected<error_code>。
auto unexpected_system_error(std::errc error) -> std::unexpected<std::error_code>
{
    return std::unexpected{ std::make_error_code(error) };
}

/// @brief 使用指定整数错误码构造 unexpected<error_code>。
/// @param[in] error 错误码。
/// @return 对应的 unexpected<error_code>。
auto unexpected_system_error(int error) -> std::unexpected<std::error_code>
{
    return std::unexpected{ std::error_code{ error, std::system_category() } };
}

} // namespace xin