#pragma once

#include <launchdarkly/server_side/integrations/big_segments/big_segment_store_types.hpp>

#include <chrono>
#include <cstddef>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_components {

/**
 * @brief Bounded, time-expiring cache of Big Segment membership lookups, keyed
 * by the unhashed context key.
 *
 * The cache holds at most a fixed number of entries; inserting beyond that
 * evicts the least-recently-used entry. Each entry also has a time-to-live.
 *
 * Expiration is lazy — there is no background reaper. An expired entry is
 * detected and dropped only when @ref Get is next called for its key (reported
 * as a miss). Until then it keeps occupying a slot, counts toward @ref Size,
 * and is eligible for normal LRU eviction like any live entry. The cache never
 * exceeds its capacity regardless, because @ref Set evicts on insert.
 *
 * Thread-safe: every method is guarded by an internal mutex and may be called
 * concurrently. Concurrent-load deduplication (querying the store once when
 * several callers miss the same key at once) is not handled here; that is the
 * caller's responsibility.
 */
class MembershipCache {
   public:
    /**
     * @param capacity Maximum number of entries retained before LRU eviction.
     * @param ttl How long an entry remains valid after insertion.
     * @param clock Source of the current time, injectable for testing.
     * Defaults to the steady clock.
     */
    MembershipCache(
        std::size_t capacity,
        std::chrono::milliseconds ttl,
        std::function<std::chrono::steady_clock::time_point()> clock = [] {
            return std::chrono::steady_clock::now();
        });

    /**
     * @brief Returns the cached membership for a context key, or nullopt if the
     * key is absent or its entry has expired. A hit refreshes the entry's LRU
     * recency but not its expiration; an expired entry is removed.
     */
    [[nodiscard]] std::optional<integrations::Membership> Get(
        std::string const& key);

    /**
     * @brief Inserts or replaces the membership for a context key, marking it
     * most-recently-used and resetting its expiration. Evicts the least-
     * recently-used entry if this pushes the cache over capacity.
     */
    void Set(std::string const& key, integrations::Membership membership);

    /**
     * @brief Removes all entries.
     */
    void Clear();

    /**
     * @brief Returns the number of entries currently held, including any that
     * have expired but not yet been pruned.
     */
    [[nodiscard]] std::size_t Size() const;

   private:
    struct Entry {
        // Position of this key in recency_, for O(1) LRU updates.
        std::list<std::string>::iterator recency_position;
        integrations::Membership membership;
        std::chrono::steady_clock::time_point expires_at;
    };

    std::size_t const capacity_;
    std::chrono::milliseconds const ttl_;
    std::function<std::chrono::steady_clock::time_point()> const clock_;

    mutable std::mutex mutex_;
    // Most-recently-used at the front, least at the back. Protected by mutex_.
    std::list<std::string> recency_;
    std::unordered_map<std::string, Entry> entries_;
};

}  // namespace launchdarkly::server_side::data_components
