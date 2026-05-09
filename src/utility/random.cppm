module;

#include <cassert>

export module xin.utility.random;

import std;

export namespace xin {

/// @brief 随机数与随机操作门面。
struct random {
    random() = delete;

    /// @brief 使用指定种子初始化线程局部随机引擎。
    /// @param value 种子值。
    static void seed(std::uint32_t value)
    {
        engine().seed(value);
    }

    /// @brief 按给定概率返回真值。
    /// @param percentage 返回 true 的概率，取值范围为 [0, 1]。
    /// @return 随机布尔值。
    static auto bernoulli(double percentage = 0.5) -> bool
    {
        std::bernoulli_distribution dist(percentage);
        return dist(engine());
    }

    /// @brief 生成半开区间 [low, high) 内的整数。
    /// @tparam T 整数类型。
    /// @param low 区间下界。
    /// @param high 区间上界，且不参与结果。
    /// @return 位于半开区间内的随机整数。
    template <std::integral T = int>
    static auto uniform(T low, T high) -> T
    {
        std::uniform_int_distribution<T> dist{ low, high - 1 };
        return dist(engine());
    }

    /// @brief 生成半开区间 [0, high) 内的整数。
    /// @tparam T 整数类型。
    /// @param high 区间上界，且不参与结果。
    /// @return 位于半开区间内的随机整数。
    template <std::integral T = int>
    static auto uniform(T high) -> T
    {
        std::uniform_int_distribution<T> dist{ T{}, high - 1 };
        return dist(engine());
    }

    /// @brief 生成半开区间 [low, high) 内的浮点数。
    /// @tparam T 浮点类型。
    /// @param low 区间下界。
    /// @param high 区间上界，且不参与结果。
    /// @return 位于半开区间内的随机浮点数。
    template <std::floating_point T = double>
    static auto uniform(T low, T high) -> T
    {
        std::uniform_real_distribution<T> dist{ low, high - std::numeric_limits<T>::epsilon() };
        return dist(engine());
    }

    /// @brief 生成半开区间 [0, high) 内的浮点数。
    /// @tparam T 浮点类型。
    /// @param high 区间上界，且不参与结果。
    /// @return 位于半开区间内的随机浮点数。
    template <std::floating_point T = double>
    static auto uniform(T high) -> T
    {
        std::uniform_real_distribution<T> dist{ T{}, high - std::numeric_limits<T>::epsilon() };
        return dist(engine());
    }

    /// @brief 生成几何分布样本。
    /// @tparam T 整数类型。
    /// @param percentage 成功概率。
    /// @return 几何分布结果。
    template <std::integral T = int>
    static auto geometric_failure(double percentage) -> T
    {
        std::geometric_distribution<T> dist(percentage);
        return dist(engine());
    }

    /// @brief 生成二项分布样本。
    /// @tparam T 整数类型。
    /// @param n 尝试次数。
    /// @param percentage 单次成功概率。
    /// @return 二项分布结果。
    template <std::integral T = int>
    static auto binomial(T n, double percentage) -> T
    {
        std::binomial_distribution<T> dist{ n, percentage };
        return dist(engine());
    }

    /// @brief 生成正态分布样本。
    /// @tparam T 浮点类型。
    /// @param mean 均值。
    /// @param stddev 标准差。
    /// @return 正态分布结果。
    template <std::floating_point T = double>
    static auto normal(T mean = 0.0, T stddev = 1.0) -> T
    {
        std::normal_distribution<T> dist{ mean, stddev };
        return dist(engine());
    }

    /// @brief 生成指数分布样本。
    /// @tparam T 浮点类型。
    /// @param lambda 速率参数。
    /// @return 指数分布结果。
    template <std::floating_point T = double>
    static auto exponential(T lambda = 1.0) -> T
    {
        std::exponential_distribution<T> dist(lambda);
        return dist(engine());
    }

    /// @brief 就地打乱区间顺序。
    /// @tparam Range 满足 std::ranges::range 的区间类型。
    /// @param range 待打乱区间。
    template <std::ranges::range Range>
    static void shuffle(Range&& range)
    {
        std::ranges::shuffle(range, engine());
    }

    /// @brief 从非空区间中随机选取一个元素。
    /// @tparam Range 满足 std::ranges::range 的区间类型。
    /// @param range 待选择区间。
    /// @return 区间中的一个随机元素引用。
    template <std::ranges::range Range>
    static auto choice(Range&& range) -> std::remove_reference_t<Range>::const_reference
    {
        assert(!std::ranges::empty(range));

        auto index = uniform(std::ranges::size(range));
        auto it = std::ranges::begin(range);
        std::advance(it, index);
        return *it;
    }

    /// @brief 从区间中抽取指定数量的样本。
    /// @tparam Range 满足 std::ranges::range 的区间类型。
    /// @param range 待采样区间。
    /// @param count 需要抽取的数量。
    /// @return 抽取出的样本集合。
    template <std::ranges::range Range>
    static auto sample(Range&& range, std::remove_reference_t<Range>::size_type count)
    {
        assert(count <= std::ranges::size(range));

        using Result = std::vector<typename std::remove_reference_t<Range>::value_type>;

        Result vec{ std::ranges::begin(range), std::ranges::end(range) };
        shuffle(vec);

        return Result{ vec.begin(), vec.begin() + count };
    }

private:
    static auto engine() -> std::mt19937&
    {
        thread_local std::mt19937 engine{ std::random_device{}() };
        return engine;
    }
};

} // namespace xin