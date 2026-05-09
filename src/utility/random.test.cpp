#include <catch2/catch_test_macros.hpp>

import std;

import xin.utility.random;


namespace {

constexpr std::uint32_t seed_value = 123456789U;
constexpr int int_low = 3;
constexpr int int_high = 9;
constexpr int int_upper = 11;
constexpr double probability_zero = 0.0;
constexpr double probability_one = 1.0;
constexpr double floating_low = 1.5;
constexpr double floating_high = 4.5;
constexpr double success_probability = 0.25;
constexpr double normal_mean = 2.5;
constexpr double normal_stddev = 0.75;
constexpr double exponential_lambda = 1.25;

} // namespace

TEST_CASE("xin::random seed 使序列可复现", "[utility][random]")
{
    xin::random::seed(seed_value);

    const auto first_int = xin::random::uniform<int>(int_upper);
    const auto first_bool = xin::random::bernoulli();
    const auto first_double = xin::random::uniform<double>(floating_high);

    xin::random::seed(seed_value);

    REQUIRE(xin::random::uniform<int>(int_upper) == first_int);
    REQUIRE(xin::random::bernoulli() == first_bool);
    REQUIRE(xin::random::uniform<double>(floating_high) == first_double);
}

TEST_CASE("xin::random bernoulli 和 uniform 满足边界约束", "[utility][random]")
{
    xin::random::seed(seed_value);

    REQUIRE_FALSE(xin::random::bernoulli(probability_zero));
    REQUIRE(xin::random::bernoulli(probability_one));

    const auto integer_value = xin::random::uniform<int>(int_low, int_high);
    REQUIRE(integer_value >= int_low);
    REQUIRE(integer_value < int_high);

    const auto integer_from_zero = xin::random::uniform<int>(int_upper);
    REQUIRE(integer_from_zero >= 0);
    REQUIRE(integer_from_zero < int_upper);

    const auto floating_value = xin::random::uniform<double>(floating_low, floating_high);
    REQUIRE(floating_value >= floating_low);
    REQUIRE(floating_value < floating_high);

    const auto floating_from_zero = xin::random::uniform<double>(floating_high);
    REQUIRE(floating_from_zero >= 0.0);
    REQUIRE(floating_from_zero < floating_high);
}

TEST_CASE("xin::random 各分布返回有效值", "[utility][random]")
{
    xin::random::seed(seed_value);

    const auto geometric_value = xin::random::geometric_failure<int>(success_probability);
    REQUIRE(geometric_value >= 0);

    const auto binomial_value = xin::random::binomial<int>(10, success_probability);
    REQUIRE(binomial_value >= 0);
    REQUIRE(binomial_value <= 10);

    const auto normal_value = xin::random::normal<double>(normal_mean, normal_stddev);
    REQUIRE(std::isfinite(normal_value));

    const auto exponential_value = xin::random::exponential<double>(exponential_lambda);
    REQUIRE(exponential_value >= 0.0);
    REQUIRE(std::isfinite(exponential_value));
}

TEST_CASE("xin::random shuffle、choice 和 sample 行为一致", "[utility][random]")
{
    xin::random::seed(seed_value);

    std::vector<int> values{ 1, 2, 3, 4, 5 };
    xin::random::shuffle(values);

    REQUIRE(std::ranges::is_permutation(values, std::vector<int>{ 1, 2, 3, 4, 5 }));

    const auto choice_value = xin::random::choice(values);
    REQUIRE(std::ranges::find(values, choice_value) != values.end());

    const auto sample_values = xin::random::sample(values, std::size_t{ 3 });
    REQUIRE(sample_values.size() == 3U);
    REQUIRE(std::ranges::all_of(sample_values, [&](const auto& item) {
        return std::ranges::find(values, item) != values.end();
    }));

    auto unique_sample_values = sample_values;
    std::ranges::sort(unique_sample_values);
    REQUIRE(std::ranges::adjacent_find(unique_sample_values) == unique_sample_values.end());
}
