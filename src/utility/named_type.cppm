export module xin.utility.named_type;

import std;

export import xin.utility.fixed_string;

export namespace xin {

/// @brief 带强类型名字的轻量值包装器，可通过 mixin skills 组合行为。
///
/// `NamedType` 用于把底层算术类型提升为语义更明确的独立类型，避免不同概念但底层类型相同的值被误用。
/// 额外行为通过 `Skills...` 组合，例如 `Arithmetic`、`Comparable`、`Hashable`、`Printable`。
///
/// ```cpp
/// using Width = xin::NamedType<int, "Width", xin::Comparable>;
/// Width width{ 42 };
/// ```
///
/// @tparam T 底层值类型。
/// @tparam Name 类型名字，通常使用字符串字面量。
/// @tparam Skills 附加行为 mixin 模板。
template <typename T, FixedString Name, template <typename> class... Skills>
    requires std::is_arithmetic_v<T>
class NamedType : public Skills<NamedType<T, Name, Skills...>>... {
public:
    using value_type = T;

    explicit constexpr NamedType(const T& v) noexcept
      : value_{ v }
    {}

    explicit constexpr NamedType(T&& v) noexcept
      : value_{ std::move(v) }
    {}

    [[nodiscard]]
    constexpr auto get() noexcept -> T&
    {
        return value_;
    }

    [[nodiscard]]
    constexpr auto get() const noexcept -> const T&
    {
        return value_;
    }

    [[nodiscard]]
    auto operator*() noexcept -> T&
    {
        return value_;
    }

    [[nodiscard]]
    auto operator*() const noexcept -> const T&
    {
        return value_;
    }

private:
    T value_;
};

/// @brief 为 `NamedType` 增加除法与除法赋值能力。
template <typename Derived>
struct Dividable {
    constexpr auto operator/=(const Derived& other) noexcept -> Derived&
    {
        static_cast<Derived*>(this)->get() /= other.get();
        return *static_cast<Derived*>(this);
    }

    [[nodiscard]]
    friend constexpr auto operator/(Derived lhs, const Derived& rhs) noexcept -> Derived
    {
        lhs /= rhs;
        return lhs;
    }
};

/// @brief 为 `NamedType` 增加前置与后置自减能力。
template <typename Derived>
struct Decrementable {
    constexpr auto operator--() noexcept -> Derived&
    {
        --static_cast<Derived*>(this)->get();
        return *static_cast<Derived*>(this);
    }

    [[nodiscard]]
    constexpr auto operator--(int) noexcept -> Derived
    {
        Derived temp = *static_cast<Derived*>(this);
        --static_cast<Derived*>(this)->get();
        return temp;
    }
};

/// @brief 为 `NamedType` 增加前置与后置自增能力。
template <typename Derived>
struct Incrementable {
    constexpr auto operator++() noexcept -> Derived&
    {
        ++static_cast<Derived*>(this)->get();
        return *static_cast<Derived*>(this);
    }

    [[nodiscard]]
    constexpr auto operator++(int) noexcept -> Derived
    {
        Derived temp = *static_cast<Derived*>(this);
        ++static_cast<Derived*>(this)->get();
        return temp;
    }
};

/// @brief 为 `NamedType` 增加加法与加法赋值能力。
template <typename Derived>
struct Addable {
    constexpr auto operator+=(const Derived& other) noexcept -> Derived&
    {
        static_cast<Derived*>(this)->get() += other.get();
        return *static_cast<Derived*>(this);
    }

    [[nodiscard]]
    friend constexpr auto operator+(Derived lhs, const Derived& rhs) noexcept -> Derived
    {
        lhs += rhs;
        return lhs;
    }
};

/// @brief 为 `NamedType` 增加减法与减法赋值能力。
template <typename Derived>
struct Subtractable {
    constexpr auto operator-=(const Derived& other) noexcept -> Derived&
    {
        static_cast<Derived*>(this)->get() -= other.get();
        return *static_cast<Derived*>(this);
    }

    [[nodiscard]]
    friend constexpr auto operator-(Derived lhs, const Derived& rhs) noexcept -> Derived
    {
        lhs -= rhs;
        return lhs;
    }
};

/// @brief 为 `NamedType` 增加乘法与乘法赋值能力。
template <typename Derived>
struct Multipliable {
    constexpr auto operator*=(const Derived& other) noexcept -> Derived&
    {
        static_cast<Derived*>(this)->get() *= other.get();
        return *static_cast<Derived*>(this);
    }

    [[nodiscard]]
    friend constexpr auto operator*(Derived lhs, const Derived& rhs) noexcept -> Derived
    {
        lhs *= rhs;
        return lhs;
    }
};

/// @brief 为 `NamedType` 增加取余与取余赋值能力。
template <typename Derived>
struct RemainderAssignable {
    constexpr auto operator%=(const Derived& other) noexcept -> Derived&
        requires std::integral<typename Derived::value_type>
    {
        static_cast<Derived*>(this)->get() %= other.get();
        return *static_cast<Derived*>(this);
    }

    constexpr auto operator%=(const Derived& other) noexcept -> Derived&
        requires std::floating_point<typename Derived::value_type>
    {
        static_cast<Derived*>(this)->get() =
            std::fmod(static_cast<Derived*>(this)->get(), other.get());
        return *static_cast<Derived*>(this);
    }

    [[nodiscard]]
    friend constexpr auto operator%(Derived lhs, const Derived& rhs) noexcept -> Derived
        requires std::integral<typename Derived::value_type> ||
                 std::floating_point<typename Derived::value_type>
    {
        lhs %= rhs;
        return lhs;
    }
};

/// @brief 聚合常见算术行为的 skill 组合。
template <typename Derived>
struct Arithmetic
  : Decrementable<Derived>
  , Incrementable<Derived>
  , Addable<Derived>
  , Subtractable<Derived>
  , Multipliable<Derived>
  , Dividable<Derived>
  , RemainderAssignable<Derived> {};

/// @brief 为 `NamedType` 增加按位与能力。
template <typename Derived>
struct BitwiseAndAssignable {
    constexpr auto operator&=(const Derived& other) noexcept -> Derived&
        requires std::integral<typename Derived::value_type>
    {
        static_cast<Derived*>(this)->get() &= other.get();
        return *static_cast<Derived*>(this);
    }

    [[nodiscard]]
    friend constexpr auto operator&(Derived lhs, const Derived& rhs) noexcept -> Derived
        requires std::integral<typename Derived::value_type>
    {
        lhs &= rhs;
        return lhs;
    }
};

/// @brief 为 `NamedType` 增加按位或能力。
template <typename Derived>
struct BitwiseOrAssignable {
    constexpr auto operator|=(const Derived& other) noexcept -> Derived&
        requires std::integral<typename Derived::value_type>
    {
        static_cast<Derived*>(this)->get() |= other.get();
        return *static_cast<Derived*>(this);
    }

    [[nodiscard]]
    friend constexpr auto operator|(Derived lhs, const Derived& rhs) noexcept -> Derived
        requires std::integral<typename Derived::value_type>
    {
        lhs |= rhs;
        return lhs;
    }
};

/// @brief 为 `NamedType` 增加按位异或能力。
template <typename Derived>
struct BitwiseXorAssignable {
    constexpr auto operator^=(const Derived& other) noexcept -> Derived&
        requires std::integral<typename Derived::value_type>
    {
        static_cast<Derived*>(this)->get() ^= other.get();
        return *static_cast<Derived*>(this);
    }

    [[nodiscard]]
    friend constexpr auto operator^(Derived lhs, const Derived& rhs) noexcept -> Derived
        requires std::integral<typename Derived::value_type>
    {
        lhs ^= rhs;
        return lhs;
    }
};

/// @brief 聚合常见按位运算行为的 skill 组合。
template <typename Derived>
struct Bitwise
  : BitwiseAndAssignable<Derived>
  , BitwiseOrAssignable<Derived>
  , BitwiseXorAssignable<Derived> {};

/// @brief 为 `NamedType` 增加比较能力。
template <typename Derived>
struct Comparable {
    [[nodiscard]]
    friend constexpr auto operator<=>(const Derived& lhs, const Derived& rhs)
    {
        return lhs.get() <=> rhs.get();
    }

    [[nodiscard]]
    friend constexpr auto operator==(const Derived& lhs, const Derived& rhs) -> bool
    {
        return lhs.get() == rhs.get();
    }
};

/// @brief 为 `NamedType` 增加基于底层值的哈希能力。
template <typename Derived>
struct Hashable {
    [[nodiscard]]
    auto hash() const noexcept -> std::size_t
    {
        using HashType = typename Derived::value_type;
        return std::hash<HashType>{}(static_cast<const Derived*>(this)->get());
    }
};

/// @brief 为 `NamedType` 增加基于底层值的流输出能力。
template <typename Derived>
struct Printable {
    friend auto operator<<(std::ostream& os, const Derived& obj) -> std::ostream&
    {
        return os << obj.get();
    }
};

} // namespace xin

template <typename T, xin::FixedString Name, template <typename> class... Skills>
struct std::hash<xin::NamedType<T, Name, Skills...>> {
    auto operator()(const xin::NamedType<T, Name, Skills...>& obj) const noexcept -> std::size_t
    {
        return obj.hash();
    }
};