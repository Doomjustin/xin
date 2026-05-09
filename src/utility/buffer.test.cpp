#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.buffer;


namespace {

constexpr std::size_t word_count{ 3 };
constexpr std::size_t dword_count{ 2 };
constexpr std::uint16_t word_0{ 0x1122U };
constexpr std::uint16_t word_1{ 0x3344U };
constexpr std::uint16_t word_2{ 0x5566U };
constexpr std::uint32_t dword_0{ 0x01020304U };
constexpr std::uint32_t dword_1{ 0xA0B0C0D0U };
constexpr std::byte patched_byte{ std::byte{ 0x7FU } };
constexpr std::string_view text_value{ "xin-buffer" };

} // namespace

TEST_CASE("xin::buffer 返回只读字节视图并保持内存一致", "[utility][buffer][const]")
{
    const std::array<std::uint16_t, word_count> values{ word_0, word_1, word_2 };

    const auto bytes = xin::buffer(values);
    const auto expected = std::as_bytes(std::span{ values });

    REQUIRE(bytes.size() == expected.size());
    REQUIRE(bytes.data() == expected.data());

    for (std::size_t i{ 0 }; i < expected.size(); ++i)
        REQUIRE(bytes[i] == expected[i]);
}

TEST_CASE("xin::buffer 返回可写字节视图并可反映到底层对象", "[utility][buffer][mutable]")
{
    std::array<std::uint32_t, dword_count> values{ dword_0, dword_1 };

    auto bytes = xin::buffer(values);
    auto expected = std::as_writable_bytes(std::span{ values });

    REQUIRE(bytes.size() == expected.size());
    REQUIRE(bytes.data() == expected.data());

    bytes.front() = patched_byte;

    expected = std::as_writable_bytes(std::span{ values });
    REQUIRE(expected.front() == patched_byte);
}

TEST_CASE("xin::buffer 支持 std::string 的只读字节视图", "[utility][buffer][string]")
{
    const std::string text{ text_value };

    const auto bytes = xin::buffer(text);

    REQUIRE(bytes.size() == text.size());
    REQUIRE(bytes.data() == reinterpret_cast<const std::byte*>(text.data()));
    REQUIRE(std::to_integer<unsigned char>(bytes.front()) ==
            static_cast<unsigned char>(text.front()));
}

TEST_CASE("xin::buffer 处理空连续区间", "[utility][buffer][empty]")
{
    std::vector<int> values{};

    const auto const_bytes = xin::buffer(std::as_const(values));
    const auto mutable_bytes = xin::buffer(values);

    REQUIRE(const_bytes.empty());
    REQUIRE(mutable_bytes.empty());
    REQUIRE(const_bytes.size() == 0U);
    REQUIRE(mutable_bytes.size() == 0U);
}

TEST_CASE("buffer 相关概念满足预期", "[utility][buffer][concepts]")
{
    using mutable_range_t = std::array<int, 4>;
    using sequence_t = std::vector<std::array<int, 2>>;
    using const_sequence_t = const std::vector<std::array<int, 2>>;

    CHECK(xin::mutable_buffer<mutable_range_t>);
    CHECK(xin::const_buffer<mutable_range_t>);
    CHECK_FALSE(xin::sequence_buffer<sequence_t>);
    CHECK(xin::sequence_buffer<const_sequence_t>);
}