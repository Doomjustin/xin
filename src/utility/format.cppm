module;

#include <format>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

#include <magic_enum/magic_enum.hpp>

export module xin.utility.format;

/// @brief 将 std::error_code 转换为其错误消息字符串。
/// @param error_code 需要转换的错误码。
/// @return 错误码对应的消息文本。
export auto format_as(const std::error_code& error_code) -> std::string
{
    return error_code.message();
}

export namespace xin {

/// @brief 约束：类型支持通过自由函数 format_as 转换为可格式化值。
/// @tparam T 待检测类型。
template <typename T>
concept has_format_as = requires(const T& value) { format_as(value); };

/// @brief 约束：类型提供成员函数 to_string。
/// @tparam T 待检测类型。
template <typename T>
concept has_to_string = requires(const T& value) { value.to_string(); };

/// @brief 约束：类型提供成员函数 to_repr。
/// @tparam T 待检测类型。
template <typename T>
concept has_to_repr = requires(const T& value) { value.to_repr(); };

/// @brief 约束：类型可通过流插入运算符写入 std::ostream。
/// @tparam T 待检测类型。
template <typename T>
concept has_ostream = requires(const T& value, std::ostream& stream) { stream << value; };

/// @brief 约束：类型是类或联合体。
/// @tparam T 待检测类型。
template <typename T>
concept user_defined_type =
    std::is_class_v<std::remove_cvref_t<T>> || std::is_union_v<std::remove_cvref_t<T>>;

} // namespace xin

/// @brief 为支持 format_as 的类型提供 std::formatter 偏特化。
/// @tparam T 支持自由函数 format_as 的类型。
template <typename T>
    requires xin::has_format_as<T>
struct std::formatter<T> : std::formatter<decltype(format_as(std::declval<T>()))> {
    /// @brief 将值先转换为 format_as 返回值，再交给基础 formatter 格式化。
    auto format(const T& value, auto& ctx) const
    {
        return std::formatter<decltype(format_as(value))>::format(format_as(value), ctx);
    }
};

/// @brief 为提供 to_string 的类型提供 std::formatter 偏特化。
/// @tparam T 提供成员函数 to_string 的类型。
template <typename T>
    requires(!xin::has_format_as<T>) && xin::has_to_string<T>
struct std::formatter<T> : std::formatter<std::string> {
    /// @brief 使用 to_string 的结果作为格式化内容。
    auto format(const T& value, auto& ctx) const
    {
        return std::formatter<std::string>::format(value.to_string(), ctx);
    }
};

/// @brief 为提供 to_repr 的类型提供 std::formatter 偏特化。
/// @tparam T 提供成员函数 to_repr 的类型。
template <typename T>
    requires(!xin::has_format_as<T>) && (!xin::has_to_string<T>) && xin::has_to_repr<T>
struct std::formatter<T> : std::formatter<std::string> {
    /// @brief 使用 to_repr 的结果作为格式化内容。
    auto format(const T& value, auto& ctx) const
    {
        return std::formatter<std::string>::format(value.to_repr(), ctx);
    }
};

/// @brief 为枚举类型提供基于 magic_enum 的 std::formatter 偏特化。
/// @tparam T 枚举类型。
template <typename T>
    requires(!xin::has_format_as<T>) && (!xin::has_to_string<T>) && (!xin::has_to_repr<T>) &&
            std::is_enum_v<T>
struct std::formatter<T> : std::formatter<std::string_view> {
    /// @brief 使用枚举项名称作为格式化内容。
    auto format(const T& value, auto& ctx) const
    {
        return std::formatter<std::string_view>::format(magic_enum::enum_name(value), ctx);
    }
};

/// @brief 为用户自定义且支持流输出的类型提供 std::formatter 偏特化。
/// @tparam T 用户自定义类型且可写入 std::ostream 的类型。
template <typename T>
    requires(!xin::has_format_as<T>) && (!xin::has_to_string<T>) && (!xin::has_to_repr<T>) &&
            xin::user_defined_type<T> && xin::has_ostream<T>
struct std::formatter<T> : std::formatter<std::string> {
    /// @brief 先写入字符串流，再将结果作为格式化内容。
    auto format(const T& value, auto& ctx) const
    {
        std::ostringstream stream;
        stream << value;
        return std::formatter<std::string>::format(stream.str(), ctx);
    }
};