export module xin.utility.cast;

import std;

export namespace xin {

/// @brief 将 ASCII 英文字母转换为大写，其他字符保持不变。
/// @param[in] input 输入字符串视图。
/// @return 转换后的新字符串。
auto to_uppercase(std::string_view input) -> std::string
{
    std::string result{ input };
    for (char& c : result)
        if (c >= 'a' && c <= 'z')
            c = static_cast<char>(c - ('a' - 'A'));
    return result;
}

/// @brief 将 ASCII 英文字母转换为小写，其他字符保持不变。
/// @param[in] input 输入字符串视图。
/// @return 转换后的新字符串。
auto to_lowercase(std::string_view input) -> std::string
{
    std::string result{ input };
    for (char& c : result)
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c + ('a' - 'A'));
    return result;
}

/// @brief 将字符串解析为数值类型。
/// @tparam T 目标数值类型。
/// @param[in] str 待解析字符串。
/// @return 成功时返回解析值，失败时返回 `std::error_code`。
/// ```cpp
/// const auto value = xin::numeric_cast<int>("42");
/// ```
template<typename T>
auto numeric_cast(std::string_view str) -> std::expected<T, std::error_code>
{
    T value{};
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

    if (ec != std::errc{})
        return std::unexpected(std::make_error_code(ec));

    if (ptr != str.data() + str.size())
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));

    return value;
}

/// @brief 将浮点值转为无分配格式化依赖的十进制字符串。
/// @tparam T 浮点类型。
/// @param[in] value 待转换浮点值。
/// @return 成功时返回字符串，失败时返回 `std::error_code`。
template<std::floating_point T>
auto string_cast(T value) -> std::expected<std::string, std::error_code>
{
    // 符号/小数点/指数("e+NNN") 余量
    constexpr std::size_t buffer_size =
        static_cast<std::size_t>(std::numeric_limits<T>::max_digits10) + 8;

    std::array<char, buffer_size> buffer{};

    auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (ec != std::errc{})
        return std::unexpected(std::make_error_code(ec));

    if (ptr == buffer.data())
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));

    return std::string(buffer.data(), static_cast<std::size_t>(ptr - buffer.data()));
}

} // namespace xin