module;

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

export module Xin.Database;

import Xin.Utility;

namespace xin {

using SequenceNumber = std::uint64_t;

export
class WriteBatch {
public:
    void put(std::string_view key, std::string_view value)
    {
        records_.emplace_back(key, value);
    }

    void remove(std::string_view key)
    {
        records_.emplace_back(key);
    }

    constexpr std::size_t size() const noexcept { return records_.size(); }

    void decode(std::ostream& os) const noexcept
    {
        os << sequence_number_ << size();

        for (const auto& record: records_)
            record.decode(os);
    }

private:
    class Record {
    public:
        Record(std::string_view key, std::string_view value)
          : type_{ Type::Value }, key_{ key }, value_{ value }
        {}

        explicit Record(std::string_view key)
          : type_{ Type::Deletion }, key_{ key }
        {}

        std::string key() const noexcept { return key_; }

        std::string value() const noexcept { return value_; }

        void decode(std::ostream& os) const noexcept
        {
            os << std::to_underlying(type_);
            os << key_.size() << key_;
            if (!is_removed())
                os << value_.size() << value_;
        }

    private:
        enum class Type: std::uint8_t {
            Value = 0x01,
            Deletion = 0x02
        };

        Type type_;
        std::string key_;
        std::string value_;

        constexpr bool is_removed() const noexcept
        {
            return type_ == Type::Deletion;
        }
    };

    SequenceNumber sequence_number_ = 0;
    std::vector<Record> records_;
};


export
class Database: NonCopyable {
public:
    explicit Database(std::string_view name)
      : name_{ name }
    {}

    virtual ~Database();

    void put(std::string_view key, std::string_view value);

    void remove(std::string_view key);

    std::optional<std::string> get(std::string_view key) const;

    virtual void write(const WriteBatch& batch) = 0;

private:
    std::string name_;
};

} // namespace xin
