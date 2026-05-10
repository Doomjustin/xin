#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.coding;

namespace {

constexpr std::uint32_t value_zero = 0U;
constexpr std::uint32_t value_one_byte_max = 127U;
constexpr std::uint32_t value_two_bytes_min = 128U;
constexpr std::uint32_t value_two_bytes_example = 300U;
constexpr std::uint16_t fixed_u16 = 0x1234U;
constexpr std::uint32_t fixed_u32 = 0x12345678U;

} // namespace

TEST_CASE("xin::varint length 返回续字节数量", "[utility][coding]")
{
    REQUIRE(xin::varint::length(value_zero) == 0U);
    REQUIRE(xin::varint::length(value_one_byte_max) == 0U);
    REQUIRE(xin::varint::length(value_two_bytes_min) == 1U);
    REQUIRE(xin::varint::length(value_two_bytes_example) == 1U);
    REQUIRE(xin::varint::length(std::uint64_t{ 16384 }) == 2U);
}

TEST_CASE("xin::varint 使用 std::byte 迭代器编解码", "[utility][coding]")
{
    std::vector<std::byte> buffer;
    xin::varint::encode(std::back_inserter(buffer), value_two_bytes_example);

    REQUIRE(buffer.size() == 2U);
    REQUIRE(buffer[0] == std::byte{ 0xAC });
    REQUIRE(buffer[1] == std::byte{ 0x02 });

    const auto [decoded, next] = xin::varint::decode<std::uint32_t>(buffer.begin());
    REQUIRE(decoded == value_two_bytes_example);
    REQUIRE(next == buffer.end());
}

TEST_CASE("xin::varint 使用 char 迭代器编解码", "[utility][coding]")
{
    std::string buffer;
    xin::varint::encode(std::back_inserter(buffer), value_two_bytes_example);

    REQUIRE(buffer.size() == 2U);
    REQUIRE(static_cast<std::uint8_t>(buffer[0]) == 0xACU);
    REQUIRE(static_cast<std::uint8_t>(buffer[1]) == 0x02U);

    const auto [decoded, next] = xin::varint::decode<std::uint32_t>(buffer.begin());
    REQUIRE(decoded == value_two_bytes_example);
    REQUIRE(next == buffer.end());
}

TEST_CASE("xin::fixed 使用 std::byte 迭代器编解码", "[utility][coding]")
{
    std::vector<std::byte> buffer;
    xin::fixed::encode(std::back_inserter(buffer), fixed_u16);

    REQUIRE(buffer.size() == sizeof(fixed_u16));
    REQUIRE(buffer[0] == std::byte{ 0x34 });
    REQUIRE(buffer[1] == std::byte{ 0x12 });

    const auto [decoded, next] = xin::fixed::decode<std::uint16_t>(buffer.begin());
    REQUIRE(decoded == fixed_u16);
    REQUIRE(next == buffer.end());
}

TEST_CASE("xin::fixed 使用 char 迭代器编解码", "[utility][coding]")
{
    std::string buffer;
    xin::fixed::encode(std::back_inserter(buffer), fixed_u32);

    REQUIRE(buffer.size() == sizeof(fixed_u32));
    REQUIRE(static_cast<std::uint8_t>(buffer[0]) == 0x78U);
    REQUIRE(static_cast<std::uint8_t>(buffer[1]) == 0x56U);
    REQUIRE(static_cast<std::uint8_t>(buffer[2]) == 0x34U);
    REQUIRE(static_cast<std::uint8_t>(buffer[3]) == 0x12U);

    const auto [decoded, next] = xin::fixed::decode<std::uint32_t>(buffer.begin());
    REQUIRE(decoded == fixed_u32);
    REQUIRE(next == buffer.end());
}

TEST_CASE("xin::pack 与 xin::unpack 互为逆操作", "[utility][coding]")
{
    std::uint32_t packed = 0U;
    packed = xin::pack(packed, xin::pack_bytes<1>(0xAAU));
    packed = xin::pack(packed, xin::pack_bytes<1>(0xBBU));

    REQUIRE(packed == 0xAABBU);

    const auto [low_byte, remaining_after_first] = xin::unpack<std::uint8_t, 1>(packed);
    REQUIRE(low_byte == 0xBBU);
    REQUIRE(remaining_after_first == 0xAAU);

    const auto [next_byte, remaining_after_second] =
        xin::unpack<std::uint8_t, 1>(remaining_after_first);

    REQUIRE(next_byte == 0xAAU);
    REQUIRE(remaining_after_second == 0U);
}
