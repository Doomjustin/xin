module;

#include <cerrno>

export module xin.utility.exceptions;

import std;

export namespace xin {

template <typename... Args>
void throw_system_error(int error, std::format_string<Args...> fmt, Args&&... args)
{
    throw std::system_error{ error, std::generic_category(),
                             std::format(fmt, std::forward<Args>(args)...) };
}

template <typename... Args> void throw_system_error(std::format_string<Args...> fmt, Args&&... args)
{
    throw std::system_error{ errno, std::generic_category(),
                             std::format(fmt, std::forward<Args>(args)...) };
}

auto unexpected_system_error() -> std::unexpected<std::error_code>
{
    return std::unexpected{ std::error_code{ errno, std::system_category() } };
}

auto unexpected_system_error(std::errc error) -> std::unexpected<std::error_code>
{
    return std::unexpected{ std::make_error_code(error) };
}

} // namespace xin