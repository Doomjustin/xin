module;

#include <optional>
#include <string>
#include <string_view>

export module Xin.Database;

export import :WriteBatch;


namespace xin {

export class Database {
public:
    void put(std::string_view key, std::string_view value);

    void remove(std::string_view key);

    std::optional<std::string> get(std::string_view key) const;

    void write(const WriteBatch& batch);

private:
    std::string name_;
};

} // namespace xin
