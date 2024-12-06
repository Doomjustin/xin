#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <sstream>

import xin.leveldb.coding;

TEMPLATE_TEST_CASE("test coding", "[leveldb]", std::uint32_t, std::uint64_t)
{
    using namespace xin::leveldb;

    SECTION("test fixed coding")
    {
        std::stringstream ss{};

        for (TestType value = 0; value < 100000; ++value)
            encode_fixed(ss, value);

        for (TestType expected = 0; expected < 100000; ++expected) {
            auto actual = decode_fixed<TestType>(ss);
            REQUIRE(actual == expected);
        }
    }

    SECTION("test varint coding")
    {
        std::stringstream ss{};

        for (TestType i = 0; i < (32 * 32); ++i) {
            auto value = (i / 32) << (i % 32);
            encode_varint(ss, value);
        }

        for (TestType i = 0; i < (32 * 32); ++i) {
            auto expected = (i / 32) << (i % 32);
            auto actual = decode_varint<TestType>(ss);

            REQUIRE(expected == actual);
        }
    }

}
