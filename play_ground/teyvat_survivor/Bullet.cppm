module;

#include <graphics.h>

export module teyvat_survivor.bullet;

import teyvat_survivor.point;
import teyvat_survivor.rect;


export class Bullet {
public:
    void position(Point position) noexcept
    {
        position_ = position;
    }

    void draw() const noexcept
    {
        solidcircle(position_.x, position_.y, radius_);
    }

    constexpr Rect collision_box() const noexcept
    {
        return Rect{ .position = position_,
                     .width = radius_,
                     .height = radius_};
    }

    void draw_collision_box() const noexcept
    {
        const auto [position, width, height] = collision_box();
        rectangle(position.x, position.y,
                    position.x + width, position.y + height);
    }

private:
    const int radius_ = 10;
    Point position_{};
};
