module;

#include <algorithm>
#include <cstdint>
#include <format>
#include <istream>
#include <optional>
#include <ostream>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

export module xin.database;

import xin.utility;
import xin.database.coding;

namespace xin::database {

export using SequenceNumber = std::uint64_t;

std::string read_string(std::istream& is)
{
    const auto size = decode_varint<std::size_t>(is);
    std::string result(size, ' ');
    is.read(result.data(), static_cast<std::streamsize>(size));

    return result;
}

export class WriteBatch {
private:
    class Record {
        friend std::ostream& operator<<(std::ostream& os, const Record& record);

    public:
        explicit Record(std::string_view key)
          : type_{ Type::Deletion },
            key_{ key }
        {}

        Record(std::string_view key, std::string_view value)
          : key_{ key },
            value_{ value }
        {}

        constexpr std::string_view key() const noexcept { return key_; }

        constexpr std::string_view value() const noexcept { return value_; }

        void encode(std::ostream& os) const noexcept
        {
            encode_fixed(os, std::to_underlying(type_));

            encode_varint(os, key_.size());
            os << key_;

            if (type_ == Type::Value) {
                encode_varint(os, value_.size());
                os << value_;
            }
        }

        constexpr bool operator==(const Record& other) const noexcept
        {
            return type_ == other.type_ &&
                   key_ == other.key_ &&
                   value_ == other.value_;
        }

        static Record decode(std::istream& is)
        {
            Record record{};
            record.type_ = static_cast<Type>(decode_fixed<std::underlying_type_t<Type>>(is));
            record.key_ = read_string(is);
            if (record.type_ == Type::Value)
                record.value_ = read_string(is);

            return record;
        }

    private:
        enum class Type: std::uint8_t {
            Value,
            Deletion
        };

        Type type_ = Type::Value;
        std::string key_;
        std::string value_;

        Record() = default;
    };

public:
    friend std::ostream& operator<<(std::ostream& os, const WriteBatch& batch);
    friend std::ostream& operator<<(std::ostream& os, const WriteBatch::Record& record);

    using SizeType = typename std::vector<Record>::size_type;

    void put(std::string_view key, std::string_view value)
    {
        records_.emplace_back(key, value);
    }

    void remove(std::string_view key)
    {
        records_.emplace_back(key);
    }

    void encode(std::ostream& os) const noexcept
    {
        encode_fixed(os, sequence_number_);
        encode_varint(os, records_.size());

        for (const auto& record: records_)
            record.encode(os);
    }

    constexpr SizeType size() const noexcept { return records_.size(); }

    std::vector<Record>::const_iterator begin() const noexcept { return records_.begin(); }

    std::vector<Record>::const_iterator end() const noexcept { return records_.end(); }

    bool operator==(const WriteBatch& other) const noexcept
    {
        if (sequence_number_ != other.sequence_number_) return false;

        if (records_.size() != other.records_.size()) return false;

        return std::ranges::equal(records_, other.records_);
    }

    WriteBatch& operator+=(const WriteBatch& other) noexcept
    {
        std::ranges::copy(other.records_, std::back_inserter(records_));
        return *this;
    }

    static WriteBatch decode(std::istream& is)
    {
        WriteBatch batch{};
        batch.sequence_number_ = decode_fixed<SequenceNumber>(is);
        auto record_size = decode_varint<SizeType>(is);

        while (record_size-- > 0)
            batch.records_.push_back(Record::decode(is));

        return batch;
    }

private:
    SequenceNumber sequence_number_ = 0;
    std::vector<Record> records_;
};


export WriteBatch operator+(WriteBatch lhs, const WriteBatch& rhs)
{
    return lhs += rhs;
}

std::ostream& operator<<(std::ostream& os, const WriteBatch::Record& record)
{
    switch (record.type_) {
    case WriteBatch::Record::Type::Value:
        std::format_to(std::ostream_iterator<char>(os), "[value]{}: {}", record.key_, record.value_);
        break;
    case WriteBatch::Record::Type::Deletion:
        std::format_to(std::ostream_iterator<char>(os), "[deleted]{}", record.key_);
        break;
    }

    return os;
}

export std::ostream& operator<<(std::ostream& os, const WriteBatch& batch)
{
    os << batch.sequence_number_ << '\n';
    std::ranges::copy(batch.records_, std::ostream_iterator<WriteBatch::Record>(os, "\n"));
    return os;
}


export class Database: NonCopyable {
public:
    explicit Database(std::string_view name)
      : name_{ name }
    {}

    virtual ~Database() = default;

    void put(std::string_view key, std::string_view value)
    {
        WriteBatch batch{};
        batch.put(key, value);
        write(std::move(batch));
    }

    void remove(std::string_view key)
    {
        WriteBatch batch{};
        batch.remove(key);
        write(std::move(batch));
    }

    virtual std::optional<std::string> get(std::string_view key) const = 0;

    virtual void write(WriteBatch batch) = 0;

private:
    std::string name_;
};


export std::unique_ptr<Database> create_database(std::string_view name, std::filesystem::path base_dir);

} // namespace xin::database
