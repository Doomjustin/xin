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

} // namespace xin