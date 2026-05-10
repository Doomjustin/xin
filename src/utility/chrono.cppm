export module xin.utility.chrono;

import std;

export namespace xin {

/// @brief 主模板，表示类型不是 chrono duration。
/// @tparam T 待检测类型。
template<typename T>
struct is_chrono_duration_impl : std::false_type {};

/// @brief 特化模板，将 std::chrono::duration 标记为 chrono duration 类型。
/// @tparam Rep 表示类型。
/// @tparam Period 时钟周期比类型。
template<typename Rep, typename Period>
struct is_chrono_duration_impl<std::chrono::duration<Rep, Period>> : std::true_type {};

/// @brief 在移除 cvref 后判断 T 是否为 std::chrono::duration。
/// @tparam T 待检测类型。
template<typename T>
concept chrono_duration = is_chrono_duration_impl<std::remove_cvref_t<T>>::value;

template<typename T>
struct is_timepoint_impl : std::false_type {};

template<typename Clock, typename Duration>
struct is_timepoint_impl<std::chrono::time_point<Clock, Duration>> : std::true_type {};

template<typename T>
concept time_point = is_timepoint_impl<std::remove_cvref_t<T>>::value;

} // namespace xin