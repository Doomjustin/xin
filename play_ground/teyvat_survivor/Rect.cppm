module;

#include <type_traits>

export module teyvat_survivor.rect;

import teyvat_survivor.point;


export template<typename T>
    requires std::is_arithmetic_v<T>
struct BasicRect {
    BasicPoint<T> position;
    int width;
    int height;
};

export using Rect = BasicRect<int>;

export template<typename T>
bool crossed(const BasicRect<T>& lhs, const BasicRect<T>& rhs)
{
    const bool x_crossed = (lhs.position.x + lhs.width) >= rhs.position.x &&
                            lhs.position.x <= (rhs.position.x + rhs.width);

    const bool y_crossed = (lhs.position.y + lhs.height) >= rhs.position.y &&
                            lhs.position.y <= (rhs.position.y + rhs.height);

    return x_crossed && y_crossed;
}
