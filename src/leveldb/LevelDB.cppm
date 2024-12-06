module;

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

export module xin.leveldb;

export import :WriteBatch;

import xin.utility;

namespace xin::leveldb {

export class LevelDB: NonCopyable {
public:
    explicit LevelDB(std::string_view name);

    virtual ~LevelDB();

    void put(std::string_view key, std::string_view value);

    void remove(std::string_view key);

    virtual std::optional<std::string> get(std::string_view key) const = 0;

    virtual void write(WriteBatch batch) = 0;

private:
    std::string name_;
};

} // namespace xin::leveldb
