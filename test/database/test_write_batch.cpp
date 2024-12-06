#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <sstream>

import xin.leveldb;

TEST_CASE("test write batch", "[database]")
{
    using namespace xin::leveldb;

    SECTION("test coding")
    {
        WriteBatch batch{};

        batch.put("key1", "value1");
        batch.put("key2", "value2");
        batch.put("key3", "value3");
        batch.put("key4", "value");
        batch.remove("key1");

        std::stringstream ss{};
        batch.encode(ss);

        auto actual = WriteBatch::decode(ss);

        REQUIRE(batch == actual);
    }

    SECTION("test operator<<")
    {
        WriteBatch batch{};
        batch.put("key1", "value1");
        batch.remove("key1");

        std::stringstream ss{};
        ss << batch;

        const std::string expected{ "0\n[value]key1: value1\n[deleted]key1\n" };
        REQUIRE_THAT(ss.str(), Catch::Matchers::Equals(expected));
    }

    SECTION("test operator+=")
    {
        WriteBatch batch1{};
        batch1.put("key1", "value1");
        batch1.remove("key1");

        WriteBatch batch2{};
        batch2.put("key2", "value2");
        batch2.remove("key3");

        batch1 += batch2;

        std::stringstream ss{};
        ss << batch1;

        const std::string expected{ "0\n[value]key1: value1\n[deleted]key1\n[value]key2: value2\n[deleted]key3\n" };
        REQUIRE_THAT(ss.str(), Catch::Matchers::Equals(expected));
    }

    SECTION("test operator+")
    {
        WriteBatch batch1{};
        batch1.put("key1", "value1");
        batch1.remove("key1");

        WriteBatch batch2{};
        batch2.put("key2", "value2");
        batch2.remove("key3");

        auto batch3 = batch1 + batch2;

        std::stringstream ss{};
        ss << batch3;

        const std::string expected{ "0\n[value]key1: value1\n[deleted]key1\n[value]key2: value2\n[deleted]key3\n" };
        REQUIRE_THAT(ss.str(), Catch::Matchers::Equals(expected));
    }
}
