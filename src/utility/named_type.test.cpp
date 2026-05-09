#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.named_type;


namespace {

using width_type = xin::NamedType<int, "Width", xin::Comparable, xin::Hashable, xin::Printable>;
using counter_type = xin::NamedType<int, "Counter", xin::Arithmetic, xin::Comparable>;
using flags_type = xin::NamedType<unsigned, "Flags", xin::Bitwise, xin::Comparable>;
using ratio_type = xin::NamedType<double, "Ratio", xin::Arithmetic, xin::Comparable>;

} // namespace

TEST_CASE("xin::NamedType 保留底层值访问", "[utility][named_type]")
{
    width_type width{ 42 };

    REQUIRE(width.get() == 42);
    REQUIRE(*width == 42);

    width.get() = 7;

    REQUIRE(width.get() == 7);
    REQUIRE(*width == 7);
}

TEST_CASE("xin::NamedType 的 Arithmetic skill 支持整数算术", "[utility][named_type]")
{
    counter_type lhs{ 9 };
    const counter_type rhs{ 4 };

    REQUIRE((lhs + rhs).get() == 13);
    REQUIRE((lhs - rhs).get() == 5);
    REQUIRE((lhs * rhs).get() == 36);
    REQUIRE((lhs / rhs).get() == 2);
    REQUIRE((lhs % rhs).get() == 1);

    lhs += rhs;
    REQUIRE(lhs.get() == 13);

    lhs -= counter_type{ 3 };
    REQUIRE(lhs.get() == 10);

    lhs *= counter_type{ 2 };
    REQUIRE(lhs.get() == 20);

    lhs /= counter_type{ 5 };
    REQUIRE(lhs.get() == 4);

    lhs %= counter_type{ 3 };
    REQUIRE(lhs.get() == 1);
}

TEST_CASE("xin::NamedType 的 Arithmetic skill 支持自增自减与浮点取余", "[utility][named_type]")
{
    counter_type counter{ 3 };

    REQUIRE((++counter).get() == 4);
    REQUIRE((counter++).get() == 4);
    REQUIRE(counter.get() == 5);
    REQUIRE((--counter).get() == 4);
    REQUIRE((counter--).get() == 4);
    REQUIRE(counter.get() == 3);

    ratio_type ratio{ 5.5 };
    ratio %= ratio_type{ 2.0 };

    REQUIRE(ratio.get() == 1.5);
    REQUIRE((ratio_type{ 5.5 } % ratio_type{ 2.0 }).get() == 1.5);
}

TEST_CASE("xin::NamedType 的 Bitwise skill 支持位运算", "[utility][named_type]")
{
    flags_type lhs{ 0b1100U };
    const flags_type rhs{ 0b1010U };

    REQUIRE((lhs & rhs).get() == 0b1000U);
    REQUIRE((lhs | rhs).get() == 0b1110U);
    REQUIRE((lhs ^ rhs).get() == 0b0110U);

    lhs &= rhs;
    REQUIRE(lhs.get() == 0b1000U);

    lhs |= flags_type{ 0b0011U };
    REQUIRE(lhs.get() == 0b1011U);

    lhs ^= flags_type{ 0b1111U };
    REQUIRE(lhs.get() == 0b0100U);
}

TEST_CASE("xin::NamedType 的 Comparable、Hashable 与 Printable skill 按底层值工作",
          "[utility][named_type]")
{
    const width_type small{ 3 };
    const width_type same{ 3 };
    const width_type large{ 8 };

    REQUIRE(small == same);
    REQUIRE(small < large);
    REQUIRE(large > same);

    REQUIRE(small.hash() == std::hash<int>{}(3));
    REQUIRE(std::hash<width_type>{}(small) == std::hash<int>{}(3));

    std::ostringstream stream;
    stream << large;
    REQUIRE(stream.str() == "8");
}
