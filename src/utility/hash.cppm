export module xin.utility.hash;

import std;

export namespace xin {

/// @brief 指示 `hash()` 使用 `std::hash<std::string_view>` 的标签类型。
struct UseStdHashT {
    std::string_view value;
};

/// @brief 指示 `hash()` 使用 FNV-1a 算法的标签类型。
struct UseFNV1aHashT {
    std::string_view value;
};

constexpr auto use_std_hash(const std::string_view value) noexcept -> UseStdHashT
{
    return UseStdHashT{ value };
}

constexpr auto use_fnv_1a(const std::string_view value) noexcept -> UseFNV1aHashT
{
    return UseFNV1aHashT{ value };
}

/// @brief 使用标准库 `std::hash<std::string_view>` 计算哈希值。
/// @param[in] value 包含待哈希字符串视图的标签对象。
/// @return `std::hash<std::string_view>` 的计算结果。
[[nodiscard]]
auto hash(const UseStdHashT value) noexcept -> std::size_t
{
    using Hasher = std::hash<std::string_view>;
    return Hasher{}(value.value);
}

[[nodiscard]]
/// @brief 使用 64 位 FNV-1a 算法计算字符串哈希值。
/// @param[in] value 包含待哈希字符串视图的标签对象。
/// @return FNV-1a 64 位哈希结果。
/// ```cpp
/// const auto digest = xin::hash(xin::use_fnv_1a("xin"));
/// ```
constexpr auto hash(const UseFNV1aHashT value) noexcept -> std::uint64_t
{
    constexpr auto FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr auto FNV_PRIME = 1099511628211ULL;

    auto result = FNV_OFFSET_BASIS;
    for (const unsigned char byte : value.value) {
        result ^= byte;
        result *= FNV_PRIME;
    }

    return result;
}

/// @brief 透明字符串哈希器，基于 FNV-1a 计算 `std::size_t` 哈希值。
struct StringHash {
    using is_transparent = void;

    auto operator()(const std::string_view sv) const noexcept -> std::size_t
    {
        return static_cast<std::size_t>(hash(use_fnv_1a(sv)));
    }
};

/// @brief 面向 `std::pmr` 容器场景的透明字符串哈希器。
struct PmrStringHash {
    using is_transparent = void;

    auto operator()(const std::string_view sv) const noexcept -> std::size_t
    {
        return static_cast<std::size_t>(hash(use_fnv_1a(sv)));
    }
};

} // namespace xin