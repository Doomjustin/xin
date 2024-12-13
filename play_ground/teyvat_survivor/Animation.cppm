module;

#include <chrono>
#include <cstddef>
#include <vector>

export module teyvat_survivor.animation;

import teyvat_survivor.common;
import teyvat_survivor.point;
import teyvat_survivor.atlas;

export class Animation {
public:
    Animation(Atlas& atlas, Duration interval)
      : atlas_{ atlas }, frame_interval_{ interval }
    {}

    void play(Point position, Duration delta)
    {
        timer_ += delta;
        if (timer_ >= frame_interval_) {
            current_index_ = (current_index_ + 1) % atlas_.images.size();
            timer_ %= frame_interval_;
        }

        put_image_alpha(position, atlas_.images[current_index_]);
    }

private:
    Atlas& atlas_;
    Duration timer_ = Duration::zero();
    Duration frame_interval_;
    std::size_t current_index_ = 0;
};
