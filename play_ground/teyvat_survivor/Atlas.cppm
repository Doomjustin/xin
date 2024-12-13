module;

#include <filesystem>
#include <graphics.h>

#include <string_view>
#include <vector>

export module teyvat_survivor.atlas;


export struct Atlas {
    explicit Atlas(std::string_view path_fmt, int num)
    {
        for (int i = 0; i < num; ++i) {
            auto real_path = std::vformat(path_fmt, std::make_format_args(i));
            IMAGE image{};
            loadimage(&image, real_path.c_str());
            images.push_back(std::move(image));
        }
    }

    std::vector<IMAGE> images;
};
