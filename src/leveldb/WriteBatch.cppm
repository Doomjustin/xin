module;

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

export module xin.leveldb:WriteBatch;

namespace xin::leveldb {

class Record {
friend constexpr bool operator==(const Record& lhs, const Record& rhs) noexcept;
friend std::ostream& operator<<(std::ostream& os, const Record& record);

public:
    explicit Record(std::string_view key);

    Record(std::string_view key, std::string_view value);

    std::string_view key() const noexcept { return key_; }

    std::string_view value() const noexcept { return value_; }

    void encode(std::ostream& os) const noexcept;

    static Record decode(std::istream& is) noexcept;

private:
    enum class Type: std::uint8_t {
        Value = 0x01,
        Deletion = 0x02
    };

    Type type_ = Type::Value;
    std::string key_;
    std::string value_;

    Record() = default;
};


export class WriteBatch {
friend WriteBatch operator+(WriteBatch lhs, const WriteBatch& rhs);
friend bool operator==(const WriteBatch& lhs, const WriteBatch& rhs);
friend std::ostream& operator<<(std::ostream& os, const WriteBatch& batch);

public:
    using SizeType = std::size_t;
    using SequenceNumber = std::uint64_t;

    void put(std::string_view key, std::string_view value) noexcept;

    void remove(std::string_view key) noexcept;

    constexpr SizeType size() const noexcept { return records_.size(); }

    constexpr SequenceNumber sequence_number() const noexcept { return sequence_number_; }

    void sequence_number(SequenceNumber new_value) noexcept { sequence_number_ = new_value; }

    void encode(std::ostream& os) const noexcept;

    WriteBatch& operator+=(const WriteBatch& other);


    static WriteBatch decode(std::istream& is) noexcept;

private:
    SequenceNumber sequence_number_ = 0;
    std::vector<Record> records_;
};

} // namespace xin::leveldb
