module;

#include <deque>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

export module xin.leveldb;

export import :WriteBatch;

import xin.utility;

namespace xin::leveldb {

struct Writer {
    WriteBatch batch;
};

export class LevelDB final: NonCopyable {
public:
    explicit LevelDB(std::string_view name);

    virtual ~LevelDB();

    void put(std::string_view key, std::string_view value);

    void remove(std::string_view key);

    std::optional<std::string> get(std::string_view key) const;

    void write(WriteBatch batch);

private:
    std::string name_;
    std::deque<Writer> writers_;
};

} // namespace xin::leveldb
