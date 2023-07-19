#pragma once

#include <list>
#include <string>
#include <unordered_map>

namespace launchdarkly::events::detail {

class LRUCache {
   public:
    /**
     * Constructs a new cache with a given capacity. When capacity is exceeded,
     * entries are evicted from the cache in LRU order.
     * @param capacity
     */
    explicit LRUCache(std::size_t capacity);

    /**
     * Adds a value to the cache; returns true if it was already there.
     * @param value Value to add.
     * @return True if the value was already in the cache.
     */
    bool Notice(std::string const& value);

    /**
     * Returns the current size of the cache.
     * @return Number of unique entries in cache.
     */
    std::size_t Size() const;

    /**
     * Clears all cache entries.
     */
    void Clear();

   private:
    using KeyList = std::list<std::string>;
    std::size_t capacity_;
    std::unordered_map<std::string, KeyList::reference> map_;
    KeyList list_;
};

}  // namespace launchdarkly::events::detail
