module;

#include <chrono>

export module teyvat_survivor.actor;

import teyvat_survivor.common;
import teyvat_survivor.point;


export class Actor {
public:
    Actor(int min_x, int max_x, int min_y, int max_y)
      : min_x_{ min_x }, max_x_{ max_x },
        min_y_{ min_y }, max_y_{ max_y }
    {}

    virtual ~Actor() = default;

    constexpr bool alive() const noexcept { return alive_; };

    void hurt() noexcept { alive_ = false; };

    virtual void draw(Duration delta) noexcept = 0;

    virtual void move(const Actor& target) noexcept = 0;

    constexpr Point position() const noexcept { return position_; }

protected:
    const int min_x_;
    const int max_x_;
    const int min_y_;
    const int max_y_;
    float speed_ = 3;
    bool alive_ = true;

    Point position_{};
    Direction direction_ = Direction::Left;
    bool move_left_ = false;
    bool move_right_ = false;
    bool move_up_ = false;
    bool move_down_ = false;

    void move() noexcept
    {
        int distance_x = 0;
        int distance_y = 0;

        if (move_up_) distance_y = -1;
        else if (move_down_) distance_y = 1;

        if (move_left_) distance_x = -1;
        else if (move_right_) distance_x = 1;

        if (const auto movement = std::sqrt(distance_x * distance_x + distance_y * distance_y);
            movement != 0) {
            const auto normalized_x = distance_x / movement;
            const auto normalized_y = distance_y / movement;

            position_.x = static_cast<int>(position_.x + normalized_x * speed_);
            position_.y = static_cast<int>(position_.y + normalized_y * speed_);
        }

        if (position_.y < min_y_) position_.y = min_y_;
        else if (position_.y > max_y_) position_.y = max_y_;

        if (position_.x < min_x_) position_.x = min_x_;
        else if (position_.x > max_x_) position_.x = max_x_;
    }
};
