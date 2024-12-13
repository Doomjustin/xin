module;

#include <type_traits>

export module teyvat_survivor.point;


export template<typename T>
    requires std::is_arithmetic_v<T>
struct BasicPoint {
    T x;
    T y;
};

export using Point = BasicPoint<int>;
