#include "membership_cache.hpp"

#include <utility>

namespace launchdarkly::server_side::data_components {

MembershipCache::MembershipCache(
    std::size_t capacity,
    std::chrono::milliseconds ttl,
    std::function<std::chrono::steady_clock::time_point()> clock)
    : capacity_(capacity), ttl_(ttl), clock_(std::move(clock)) {}

std::optional<integrations::Membership> MembershipCache::Get(
    std::string const& key) {
    std::lock_guard lock(mutex_);

    auto const it = entries_.find(key);
    if (it == entries_.end()) {
        return std::nullopt;
    }

    if (clock_() >= it->second.expires_at) {
        recency_.erase(it->second.recency_position);
        entries_.erase(it);
        return std::nullopt;
    }

    // Move to front as most-recently-used.
    recency_.splice(recency_.begin(), recency_, it->second.recency_position);
    return it->second.membership;
}

void MembershipCache::Set(std::string const& key,
                          integrations::Membership membership) {
    std::lock_guard lock(mutex_);

    auto const expires_at = clock_() + ttl_;

    auto const it = entries_.find(key);
    if (it != entries_.end()) {
        recency_.splice(recency_.begin(), recency_,
                        it->second.recency_position);
        it->second.membership = std::move(membership);
        it->second.expires_at = expires_at;
        return;
    }

    recency_.push_front(key);
    entries_.emplace(
        key, Entry{recency_.begin(), std::move(membership), expires_at});

    if (entries_.size() > capacity_) {
        auto const& lru_key = recency_.back();
        entries_.erase(lru_key);
        recency_.pop_back();
    }
}

void MembershipCache::Clear() {
    std::lock_guard lock(mutex_);
    entries_.clear();
    recency_.clear();
}

std::size_t MembershipCache::Size() const {
    std::lock_guard lock(mutex_);
    return entries_.size();
}

}  // namespace launchdarkly::server_side::data_components
