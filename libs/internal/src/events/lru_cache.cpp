#include <launchdarkly/events/detail/lru_cache.hpp>

namespace launchdarkly::events::detail {
LRUCache::LRUCache(std::size_t capacity)
    : capacity_(capacity), map_(), list_() {}

bool LRUCache::Notice(std::string const& value) {
    auto it = map_.find(value);
    if (it != map_.end()) {
        list_.remove(value);
        list_.push_front(value);
        return true;
    }
    while (map_.size() >= capacity_) {
        map_.erase(list_.back());
        list_.pop_back();
    }
    list_.push_front(value);
    map_.emplace(value, list_.front());
    return false;
}

void LRUCache::Clear() {
    map_.clear();
    list_.clear();
}

std::size_t LRUCache::Size() const {
    return list_.size();
}

}  // namespace launchdarkly::events::detail
