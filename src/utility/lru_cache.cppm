module;

#include <cassert>

#include <gsl/gsl>

export module xin.utility.lru_cache;

import std;

import xin.utility.hash;

export namespace xin {

/// @brief 基于最近最少使用（LRU）策略的定长缓存。
///
/// 缓存内部维护一条访问顺序链表：链表头部表示最近访问，尾部表示最久未使用。
/// `put()` 和命中的 `get()` 都会把对应元素移动到头部；当容量已满时，插入新元素会淘汰尾部元素。
///
/// ```cpp
/// xin::LRUCache<std::string, int> cache{ 2 };
/// cache.put("a", 1);
/// cache.put("b", 2);
/// cache.get("a");
/// cache.put("c", 3); // 淘汰 "b"
/// ```
///
/// @tparam Key 键类型。
/// @tparam Value 值类型。
/// @tparam KeyHash 键哈希器类型。
/// @tparam KeyEqual 键相等比较器类型。
template<typename Key, typename Value, typename KeyHash = StringHash,
         typename KeyEqual = std::equal_to<Key>>
class LRUCache {
public:
    using resource = std::pmr::memory_resource;
    using size_type = std::size_t;
    using value_type = std::pair<const Key, Value>;
    using container = std::pmr::list<value_type>;
    using iterator = typename container::iterator;
    using const_iterator = typename container::const_iterator;
    using cache = std::pmr::unordered_map<Key, iterator, KeyHash, KeyEqual>;

    /// @brief 构造指定容量的 LRU 缓存。
    /// @param[in] capacity 缓存容量，必须大于 0。
    /// @param[in] memory_resource `std::pmr` 容器使用的内存资源。
    explicit LRUCache(size_type capacity,
                      gsl::not_null<resource*> memory_resource = std::pmr::get_default_resource())
      : capacity_{ capacity }
      , items_{ memory_resource.get() }
      , cache_{ memory_resource.get() }
    {
        assert(capacity_ > 0);
    }

    LRUCache(const LRUCache&) = delete;
    auto operator=(const LRUCache&) -> LRUCache& = delete;

    LRUCache(LRUCache&&) = default;
    auto operator=(LRUCache&&) -> LRUCache& = default;

    ~LRUCache() = default;

    /// @brief 插入或更新缓存项，并将其标记为最近使用。
    /// @param[in] key 键。
    /// @param[in] value 值。
    template<typename K, typename V>
        requires std::constructible_from<Key, K> && std::constructible_from<Value, V>
    void put(K&& key, V&& value)
    {
        if (capacity_ == 0)
            return;

        auto it = cache_.find(key);
        if (it != cache_.end()) {
            it->second->second = std::forward<V>(value);
            items_.splice(items_.begin(), items_, it->second);
            return;
        }

        if (cache_.size() == capacity_) {
            cache_.erase(items_.back().first);
            items_.pop_back();
        }

        items_.emplace_front(std::forward<K>(key), std::forward<V>(value));
        cache_.emplace(items_.begin()->first, items_.begin());
    }

    /// @brief 查找缓存项；命中时返回值引用并提升为最近使用。
    /// @param[in] key 待查找键。
    /// @return 命中时返回值的 `std::reference_wrapper`，未命中返回 `std::nullopt`。
    template<typename K>
    auto get(const K& key) noexcept -> std::optional<std::reference_wrapper<Value>>
    {
        auto it = cache_.find(key);
        if (it == cache_.end())
            return {};

        items_.splice(items_.begin(), items_, it->second);
        return std::ref(it->second->second);
    }

    /// @brief 清空缓存中的所有元素。
    void clear() noexcept
    {
        items_.clear();
        cache_.clear();
    }

    [[nodiscard]]
    constexpr auto capacity() const noexcept -> size_type
    {
        return capacity_;
    }

    [[nodiscard]]
    constexpr auto size() const noexcept -> size_type
    {
        return items_.size();
    }

    auto begin() noexcept -> iterator
    {
        return items_.begin();
    }
    auto end() noexcept -> iterator
    {
        return items_.end();
    }
    [[nodiscard]] auto begin() const noexcept -> const_iterator
    {
        return items_.begin();
    }
    [[nodiscard]] auto end() const noexcept -> const_iterator
    {
        return items_.end();
    }

    auto cbegin() noexcept -> const_iterator
    {
        return items_.cbegin();
    }
    auto cend() noexcept -> const_iterator
    {
        return items_.cend();
    }
    [[nodiscard]] auto cbegin() const noexcept -> const_iterator
    {
        return items_.cbegin();
    }
    [[nodiscard]] auto cend() const noexcept -> const_iterator
    {
        return items_.cend();
    }

private:
    size_type capacity_;
    container items_;
    cache cache_;
};

} // namespace xin