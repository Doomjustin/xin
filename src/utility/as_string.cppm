export module xin.utility.as_string;

import std;

export namespace xin {

/// @brief 将只读字节序列按原内存解释为字符串视图。
/// @param data 连续的只读字节序列。
/// @return 指向同一段内存的非拥有 std::string_view。
/// @pre data.data() 指向的内存区域在返回视图的整个使用期内保持有效。
/// @note 本函数不进行拷贝、不进行编码转换。
/// @warning 该视图可能包含 '\0' 或非文本字节，调用方需自行保证语义正确性。
auto as_string(std::span<const std::byte> data) -> std::string_view
{
    return { reinterpret_cast<const char*>(data.data()), data.size() };
}

/// @brief 将可写字节序列按原内存解释为字符串视图。
/// @param data 连续的可写字节序列。
/// @return 指向同一段内存的非拥有 std::string_view。
/// @pre data.data() 指向的内存区域在返回视图的整个使用期内保持有效。
/// @note 本函数不进行拷贝、不进行编码转换。
/// @warning 该视图可能包含 '\0' 或非文本字节，调用方需自行保证语义正确性。
auto as_string(std::span<std::byte> data) -> std::string_view
{
    return { reinterpret_cast<const char*>(data.data()), data.size() };
}

} // namespace xin