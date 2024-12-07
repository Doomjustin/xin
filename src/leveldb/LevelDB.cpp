module;

#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>

module xin.leveldb;

namespace xin::leveldb {

LevelDB::LevelDB(std::string_view name)
  : name_{ name }
{}

LevelDB::~LevelDB() = default;

void LevelDB::put(std::string_view key, std::string_view value)
{
    WriteBatch batch{};
    batch.put(key, value);
    write(std::move(batch));
}

void LevelDB::remove(std::string_view key)
{
    WriteBatch batch{};
    batch.remove(key);
    write(std::move(batch));
}

std::optional<std::string> LevelDB::get(std::string_view key) const
{
    return {};
}

void LevelDB::write(WriteBatch batch)
{

}

} // namespace xin::leveldb
