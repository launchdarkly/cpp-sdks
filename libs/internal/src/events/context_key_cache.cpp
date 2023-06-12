#include <launchdarkly/events/context_key_cache.hpp>

namespace launchdarkly::events {
ContextKeyCache::ContextKeyCache(std::size_t capacity)
    : capacity_(capacity), map_(), list_() {}

bool ContextKeyCache::Notice(std::string const& context_key) {
    auto it = map_.find(context_key);
    if (it != map_.end()) {
        list_.remove(context_key);
        list_.push_front(context_key);
        return true;
    }
    while (map_.size() >= capacity_) {
        map_.erase(list_.back());
        list_.pop_back();
    }
    list_.push_front(context_key);
    map_.emplace(context_key, list_.front());
    return false;
}

void ContextKeyCache::Clear() {
    map_.clear();
    list_.clear();
}

std::size_t ContextKeyCache::Size() const {
    return list_.size();
}

}  // namespace launchdarkly::events
