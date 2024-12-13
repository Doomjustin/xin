module;

#include <graphics.h>

#include <array>
#include <chrono>
#include <filesystem>
#include <thread>
#include <numbers>

export module teyvat_survivor.player;

import teyvat_survivor.animation;
import teyvat_survivor.common;
import teyvat_survivor.actor;
import teyvat_survivor.enemy;
import teyvat_survivor.rect;
import teyvat_survivor.point;
import teyvat_survivor.bullet;
import teyvat_survivor.atlas;

constexpr int animation_num = 6;
constexpr int animation_width = 80;
constexpr int animation_height = 80;
constexpr int shadow_width = 32;

double to_radian(double angle)
{
    return angle * std::numbers::pi / 180;
}

Point cal_bullet_position(Point center, double angle, int radius)
{
    center.x += static_cast<int>(radius * cos(to_radian(angle)));
    center.y += static_cast<int>(radius * sin(to_radian(angle)));

    return center;
}


export class Player final: public Actor {
public:
    explicit Player(Point position)
      : Actor{ 0, width - animation_width, 0, height - animation_height},
        left_animation_{ player_left_atlas_, Duration{ 45 } },
        right_animation_{player_right_atlas_, Duration{ 45 } },
        shadow_{ load_image("resource/img/shadow_player.png") }
    {
        speed_ = 3;
        position_ = position;
    }

    ~Player() = default;

    void draw(Duration delta) noexcept override
    {
        constexpr double rotate_speed = 0.5;
        constexpr double min_radius = 30;
        constexpr double max_radius = 80;

        static double angle = 0;
        angle += (rotate_speed * delta.count());
        if (angle > 360) angle -= 360;

        static double change_radius = min_radius;
        static double change_speed = 0.1;

        if (change_radius > max_radius || change_radius < min_radius)
            change_speed = -change_speed;

        change_radius += (change_speed * delta.count());

        const Point center{ position_.x + animation_width / 2, position_.y + animation_height / 2 };
        const auto base_radius = static_cast<int>(animation_width / 2.0 + change_radius);

        for (auto& bullet: bullets_) {
            bullet.position(cal_bullet_position(center, angle, base_radius));
            angle += 360.0 / bullets_.size();
            bullet.draw();
        }

        draw_shadow();
        draw_player(delta);
    }

    void move(const Actor& target) noexcept override
    {
        // TODO: 可以被分离到movement中吗？
        Actor::move();
    }

    void process_input(const ExMessage& message) noexcept
    {
        const auto type = message.message;;

        switch (const auto key = static_cast<Key>(message.vkcode)) {
            using enum Key;
        case Up: case W:
            move_up_ = type != WM_KEYUP;
            break;
        case Down: case S:
            move_down_ = type != WM_KEYUP;
            break;
        case Left: case A:
            move_left_ = type != WM_KEYUP;
            direction_ = Direction::Left;
            break;
        case Right: case D:
            move_right_ = type != WM_KEYUP;
            direction_ = Direction::Right;
            break;
        default:
            break;
        }
    }

    constexpr Rect collision_box() const noexcept
    {
        return Rect{ .position = Point{ position_.x + 25, position_.y + 25 },
                     .width = 30,
                     .height = 45 };
    }

    void draw_collision_box() const noexcept
    {
        const auto [position, width, height] = collision_box();
        rectangle(position.x, position.y,
                    position.x + width, position.y + height);

        for (const auto& bullet: bullets_)
            bullet.draw_collision_box();
    }

    bool check_collision(const Enemy& enemy) const noexcept
    {
        return crossed(collision_box(), enemy.collision_box());
    }

    bool check_attack(const Enemy& enemy) const noexcept
    {
        const auto enemy_box = enemy.collision_box();
        return std::ranges::any_of(bullets_,
            [&enemy_box] (const auto& bullet) { return crossed(bullet.collision_box(), enemy_box); });
    }

private:
    static Atlas player_left_atlas_;
    static Atlas player_right_atlas_;

    Animation left_animation_;
    Animation right_animation_;
    IMAGE shadow_;
    std::array<Bullet, 3> bullets_{};

    void draw_shadow() noexcept
    {
        const Point shadow_position{ position_.x + (animation_width / 2 - shadow_width / 2),
                                   position_.y + animation_height - 8 };
        put_image_alpha(shadow_position, shadow_);
    }

    void draw_player(Duration delta) noexcept
    {
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
};

Atlas Player::player_left_atlas_{ "resource/img/player_left_{}.png", animation_num };
Atlas Player::player_right_atlas_{ "resource/img/player_right_{}.png", animation_num };
