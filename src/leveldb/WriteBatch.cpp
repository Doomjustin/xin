module;

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string_view>
#include <vector>

module xin.leveldb;

import xin.leveldb.coding;

namespace xin::leveldb {

WriteBatch operator+(WriteBatch lhs, const WriteBatch& rhs)
{
    return lhs += rhs;
}

bool operator==(const WriteBatch& lhs, const WriteBatch& rhs)
{
    if (lhs.sequence_number_ != rhs.sequence_number_) return false;

    if (lhs.records_.size() != rhs.records_.size()) return false;

    return std::ranges::equal(lhs.records_, rhs.records_);
}

std::ostream& operator<<(std::ostream& os, const WriteBatch& batch)
{
    os << batch.sequence_number_ << '\n';
    std::ranges::copy(batch.records_, std::ostream_iterator<Record>(os, "\n"));
    return os;
}


void WriteBatch::put(std::string_view key, std::string_view value) noexcept
{
    records_.emplace_back(key, value);
}

void WriteBatch::remove(std::string_view key) noexcept
{
    records_.emplace_back(key);
}

void WriteBatch::encode(std::ostream& os) const noexcept
{
    encode_fixed(os, sequence_number_);
    encode_varint(os, records_.size());

    std::ranges::for_each(records_, [&os] (const auto& record) { record.encode(os); });
}

WriteBatch& WriteBatch::operator+=(const WriteBatch& other)
{
    std::ranges::copy(other.records_, std::back_inserter(records_));
    return *this;
}

WriteBatch WriteBatch::decode(std::istream& is) noexcept
{
    WriteBatch batch{};
    batch.sequence_number_ = decode_fixed<SequenceNumber>(is);
    auto record_size = leveldb::decode_varint<SizeType>(is);

    while (record_size-- > 0)
        batch.records_.push_back(Record::decode(is));

    return batch;
}

} // namespace xin::leveldb
