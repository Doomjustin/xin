export module xin.utility.buffer;

import std;

export namespace xin {

/// @brief 将连续只读区间转换为只读字节视图。
/// @tparam T 满足 std::ranges::contiguous_range 的区间类型。
/// @param[in] range 输入区间，底层存储需为连续内存。
/// @return 与输入区间共享同一段内存的 std::span<const std::byte>。
/// @pre range 的底层对象在返回视图的使用期内保持有效。
/// @note 本函数不分配内存、不拷贝数据。
/// ```cpp
/// std::vector<int> values{ 1, 2, 3 };
/// auto bytes = xin::buffer(values);
/// ```
template <std::ranges::contiguous_range T>
auto buffer(const T& range) noexcept -> std::span<const std::byte>
{
    return std::as_bytes(std::span{ range });
}

/// @brief 将连续可写区间转换为可写字节视图。
/// @tparam T 满足 std::ranges::contiguous_range 且元素为可修改引用的区间类型。
/// @param[in] range 输入区间，底层存储需为连续内存。
/// @return 与输入区间共享同一段内存的 std::span<std::byte>。
/// @pre range 的底层对象在返回视图的使用期内保持有效。
/// @note 本函数不分配内存、不拷贝数据。
template <std::ranges::contiguous_range T>
    requires(!std::is_const_v<std::remove_reference_t<std::ranges::range_reference_t<T>>>)
auto buffer(T& range) noexcept -> std::span<std::byte>
{
    return std::as_writable_bytes(std::span{ range });
}

/// @brief 约束：类型可通过 buffer 生成可写字节视图。
/// @tparam T 待检测类型。
template <typename T>
concept mutable_buffer = requires(T& sequence) {
    { buffer(sequence) } -> std::same_as<std::span<std::byte>>;
};

/// @brief 约束：类型可通过 buffer 生成只读字节视图。
/// @tparam T 待检测类型。
template <typename T>
concept const_buffer = requires(const T& sequence) {
    { buffer(sequence) } -> std::same_as<std::span<const std::byte>>;
};

/// @brief 约束：类型是区间，且其元素类型满足 const_buffer。
/// @tparam T 待检测区间类型。
template <typename T>
concept sequence_buffer = std::ranges::range<T> && const_buffer<std::ranges::range_reference_t<T>>;

} // namespace xin