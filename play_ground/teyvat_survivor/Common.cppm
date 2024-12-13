module;

#include <graphics.h>

#include <chrono>
#include <string_view>

export module teyvat_survivor.common;

import teyvat_survivor.point;


constexpr int key_w = 0x57;
constexpr int key_s = 0x53;
constexpr int key_a = 0x41;
constexpr int key_d = 0x44;

export constexpr int width = 1280;
export constexpr int height = 720;

export using Clock = std::chrono::steady_clock;
export using Duration = std::chrono::milliseconds;

export enum class Key {
    W = 0x57,
    S = 0x53,
    A = 0x41,
    D = 0x44,
    Up = VK_UP,
    Down = VK_DOWN,
    Left = VK_LEFT,
    Right = VK_RIGHT,
};

export enum class Direction {
    Left,
    Right,
    Up,
    Down
};

export IMAGE load_image(std::string_view path)
{
    IMAGE image{};
    loadimage(&image, path.data());
    return image;
}

export void put_image_alpha(Point position, IMAGE& image)
{
    const auto width = image.getwidth();
    const auto height = image.getheight();

    AlphaBlend(GetImageHDC(nullptr), position.x, position.y, width, height,
                GetImageHDC(&image), 0, 0, width, height,
                { AC_SRC_OVER, 0, 255 , AC_SRC_ALPHA});
}
