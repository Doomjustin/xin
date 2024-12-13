module;

#include <graphics.h>

#include <cstdint>
#include <cstdlib>
#include <memory>

export module teyvat_survivor.enemy;

import teyvat_survivor.animation;
import teyvat_survivor.actor;
import teyvat_survivor.common;
import teyvat_survivor.rect;
import teyvat_survivor.point;
import teyvat_survivor.atlas;

constexpr int animation_num = 6;
constexpr int animation_width = 80;
constexpr int animation_height = 80;
constexpr int shadow_width = 32;

export class Enemy final: public Actor {
public:
    explicit Enemy(Point position)
      : Actor{ -animation_width, width, -animation_height, height },
        left_animation_{ enemy_left_atlas_, Duration{ 70 } },
        right_animation_{ enemy_right_atlas_, Duration{ 70 } },
        shadow_{ load_image("resource/img/shadow_player.png") }
    {
        speed_ = 2;
        position_ = position;
    }

    ~Enemy() = default;

    void draw(Duration delta) noexcept override
    {
        draw_shadow();

        switch (direction_) {
        case Direction::Left:
        default:
            left_animation_.play(position_, delta);
            break;
        case Direction::Right:
            right_animation_.play(position_, delta);
            break;
        }
    }

    void move(const Actor& target) noexcept override
    {
        if (position_.x < target.position().x) {
            move_right_ = true;
            move_left_ = false;
            direction_ = Direction::Right;
        }
        else if (position_.x > target.position().x) {
            move_right_ = false;
            move_left_ = true;
            direction_ = Direction::Left;
        }
        else {
            move_right_ = false;
            move_left_ = false;
        }

        if (position_.y < target.position().y) {
            move_down_ = true;
            move_up_ = false;
        }
        else if (position_.y > target.position().y) {
            move_down_ = false;
            move_up_ = true;
        }
        else {
            move_down_ = false;
            move_up_ = false;
        }

        Actor::move();
    }

    constexpr Rect collision_box() const noexcept
    {
        return Rect{ .position = Point{ position_.x + 10, position_.y + 15 },
                     .width = 65,
                     .height = 50 };
    }

    void draw_collision_box() const noexcept
    {
        const auto [position, width, height] = collision_box();
        rectangle(position.x, position.y,
                    position.x + width, position.y + height);
    }

private:
    static Atlas enemy_left_atlas_;
    static Atlas enemy_right_atlas_;

    Animation left_animation_;
    Animation right_animation_;
    IMAGE shadow_;

    void draw_shadow() noexcept
    {
        const Point shadow_position{ position_.x + (animation_width / 2 - shadow_width / 2),
                                   position_.y + animation_height - 20 };
        put_image_alpha(shadow_position, shadow_);
    }
};

Atlas Enemy::enemy_left_atlas_{ "resource/img/enemy_left_{}.png", animation_num };
Atlas Enemy::enemy_right_atlas_{ "resource/img/enemy_right_{}.png", animation_num };


enum class SpawnEdge: std::uint8_t {
    Up,
    Down,
    Left,
    Right
};

export std::unique_ptr<Enemy> generate_single_enemy()
{
    Point start_position{};
    switch (const auto start_edge = static_cast<SpawnEdge>(std::rand() % 4)) {
    case SpawnEdge::Up:
        start_position.x = std::rand() % (width - animation_width);
        start_position.y = -animation_height;
        break;
    case SpawnEdge::Down:
        start_position.x = std::rand() % (width - animation_width);
        start_position.y = height;
        break;
    case SpawnEdge::Left:
        start_position.x = -animation_width;
        start_position.y = std::rand() % (height - animation_height);
        break;
    case SpawnEdge::Right:
        start_position.x = width;
        start_position.y = std::rand() % (height - animation_height);
        break;
    }

    return std::make_unique<Enemy>(start_position);
}
