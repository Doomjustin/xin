export module xin.utility.as_string;

import std;

export namespace xin {

auto as_string(std::span<const std::byte> data) -> std::string_view
{
    return { reinterpret_cast<const char*>(data.data()), data.size() };
}

auto as_string(std::span<std::byte> data) -> std::string_view
{
    return { reinterpret_cast<const char*>(data.data()), data.size() };
}

} // namespace xin