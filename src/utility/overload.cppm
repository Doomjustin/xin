export module xin.utility.overload;

import std;

export namespace xin {

/// @brief 将多个可调用对象聚合为一个重载集。
///
/// 常用于 `std::visit` 等需要单一 visitor 对象、但希望按参数类型分发到多个 lambda 的场景。
///
/// ```cpp
/// auto visitor = xin::Overload{
///     [](int value) { return value + 1; },
///     [](std::string_view text) { return text.size(); },
/// };
/// ```
///
/// @tparam T 可调用对象类型列表。
template <typename... T>
struct Overload : T... {
    using T::operator()...;
};

/// @brief `Overload` 的 class template argument deduction guide。
/// @tparam T 可调用对象类型列表。
template <typename... T>
Overload(T...) -> Overload<T...>;

} // namespace xin