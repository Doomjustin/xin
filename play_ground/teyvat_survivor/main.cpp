#include <graphics.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <thread>

#pragma comment(lib, "Msimg32.lib")
#pragma comment(lib, "winmm.lib")

import teyvat_survivor.common;
import teyvat_survivor.player;
import teyvat_survivor.enemy;
import teyvat_survivor.actor;
import teyvat_survivor.point;

std::vector<std::unique_ptr<Enemy>> enemies;

constexpr bool should_draw_collision_box = false;

static int score = 0;

void generate_enemy()
{
    static int counter = 0;

    if (counter++ % 60 == 0 && enemies.size() < 12) {
        enemies.push_back(generate_single_enemy());
        counter = 0;
    }
}

void draw_enemies(Duration delta, const Actor& target)
{
    std::erase_if(enemies, [] (const auto& actor) { return !actor->alive(); });

    for (const auto& enemy: enemies) {
        enemy->move(target);
        enemy->draw(delta);

        if (should_draw_collision_box)
            enemy->draw_collision_box();
    }
}

bool check_collision(const Player& player)
{
    for (const auto& enemy: enemies) {
        if (player.check_collision(*enemy)) {
            MessageBox(GetHWnd(), _T("You died"), _T("game over"), MB_OK);
            return false;
        }
    }

    return true;
}

void check_attack(const Player& player) noexcept
{
    for (auto& enemy: enemies) {
        if (player.check_attack(*enemy)) {
            mciSendString("play hit from 0", nullptr, 0, nullptr);
            enemy->hurt();
            ++score;
        }
    }
}


int main(int argc, char* argv[])
{
    initgraph(width, height);

    const auto background = load_image("resource/img/background.png");

    mciSendString("open resource/mus/bgm.mp3 alias bgm", nullptr, 0, nullptr);
    mciSendString("open resource/mus/hit.wav alias hit", nullptr, 0, nullptr);

    mciSendString("play bgm repeat from 0", nullptr, 0, nullptr);

    ExMessage message{};
    Player player{ Point{ width / 2, height / 2 } };
    bool should_exit = false;

    BeginBatchDraw();

    settextcolor(RGB(224, 175, 45));

    while (!should_exit) {
        const auto start_time = Clock::now();

        while (peekmessage(&message, EX_KEY)) {
            if (message.message == WM_KEYUP && message.vkcode == VK_ESCAPE)
                should_exit = true;

            player.process_input(message);
        }

        cleardevice();

        putimage(0, 0, &background);

        outtextxy(0, 0, std::format("current score: {}", score).data());

        using namespace std::chrono_literals;
        constexpr int frame_ratio = 120;
        constexpr auto frame_time = 1000ms / frame_ratio;

        player.move(player);
        player.draw(frame_time);
        if (should_draw_collision_box)
            player.draw_collision_box();

        draw_enemies(frame_time, player);

        check_attack(player);

        if (!check_collision(player))
            should_exit = true;

        generate_enemy();

        FlushBatchDraw();

        if (const auto delta_time = Clock::now() - start_time; delta_time < frame_time)
            std::this_thread::sleep_for(frame_time - delta_time);
    }

    EndBatchDraw();
    return EXIT_SUCCESS;
}
