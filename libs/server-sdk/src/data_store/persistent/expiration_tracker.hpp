#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <launchdarkly/connection.hpp>

#include "../../data_store/data_kind.hpp"

namespace launchdarkly::server_side::data_store::persistent {

class ExpirationTracker {
   public:
    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

    /**
     * The state of the key in the tracker.
     */
    enum class TrackState {
        /**
         * The key is tracked and the TTL has not expired.
         */
        kFresh,
        /**
         * The key is tracked and the TTL has expired.
         */
        kStale,
        /**
         * The key is not being tracked.
         */
        kNotTracked
    };

    /**
     * Add an unscoped key to the tracker.
     *
     * @param key The key to track.
     * @param expiration The time that the key expires.
     * used.
     */
    void Add(std::string const& key, TimePoint expiration);

    /**
     * Remove an unscoped key from the tracker.
     *
     * @param key The key to stop tracking.
     */
    void Remove(std::string const& key);

    /**
     * Check the state of an unscoped key.
     *
     * @param key The key to check.
     * @param current_time The current time.
     * @return The state of the key.
     */
    TrackState State(std::string const& key, TimePoint current_time);

    /**
     * Add a scoped key to the tracker. Will use the specified TTL for the kind.
     *
     * @param kind The scope (kind) of the key.
     * @param key The key to track.
     * @param expiration The time that the key expires.
     */
    void Add(data_store::DataKind kind,
             std::string const& key,
             TimePoint expiration);

    /**
     * Remove a scoped key from the tracker.
     *
     * @param kind The scope (kind) of the key.
     * @param key The key to stop tracking.
     */
    void Remove(data_store::DataKind kind, std::string const& key);

    /**
     * Check the state of a scoped key.
     *
     * @param kind The scope (kind) of the key.
     * @param key The key to check.
     * @return The state of the key.
     */
    TrackState State(data_store::DataKind kind,
                     std::string const& key,
                     TimePoint current_time);

    /**
     * Stop tracking all keys.
     */
    void Clear();

    /**
     * Prune expired keys from the tracker.
     * @param current_time The current time.
     * @return A list of all the kinds and associated keys that expired.
     * Unscoped keys will have std::nullopt as the kind.
     */
    std::vector<std::pair<std::optional<DataKind>, std::string>> Prune(
        TimePoint current_time);

   private:
    using TtlMap = std::unordered_map<std::string, TimePoint>;

    TtlMap unscoped_;

    class ScopedTtls {
       public:
        void Set(DataKind kind, std::string const& key, TimePoint expiration);
        void Remove(DataKind kind, std::string);
        void Clear();

       private:
        std::array<TtlMap, static_cast<std::size_t>(DataKind::kKindCount)>
            scoped;
    };

    ScopedTtls scoped_;
};

}  // namespace launchdarkly::server_side::data_store::persistent
