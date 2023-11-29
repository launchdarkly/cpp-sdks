#pragma once

#include "../dependency_tracker/data_kind.hpp"
#include "../dependency_tracker/tagged_data.hpp"

#include <array>
#include <chrono>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace launchdarkly::server_side::data_components {

class ExpirationTracker {
   public:
    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

    /**
     * The state of the key in the tracker.
     */
    enum class TrackState {
        /**
         * The key is tracked and the key expiration is in the future.
         */
        kFresh,
        /**
         * The key is tracked and the expiration is either now or in the past.
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
    TrackState State(std::string const& key, TimePoint current_time) const;

    /**
     * Add a scoped key to the tracker. Will use the specified TTL for the kind.
     *
     * @param kind The scope (kind) of the key.
     * @param key The key to track.
     * @param expiration The time that the key expires.
     */
    void Add(DataKind kind, std::string const& key, TimePoint expiration);

    /**
     * Remove a scoped key from the tracker.
     *
     * @param kind The scope (kind) of the key.
     * @param key The key to stop tracking.
     */
    void Remove(DataKind kind, std::string const& key);

    /**
     * Check the state of a scoped key.
     *
     * @param kind The scope (kind) of the key.
     * @param key The key to check.
     * @return The state of the key.
     */
    TrackState State(DataKind kind,
                     std::string const& key,
                     TimePoint current_time) const;

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

    static TrackState State(TimePoint expiration, TimePoint current_time);

    class ScopedTtls {
       public:
        ScopedTtls();

        using DataType =
            std::array<TaggedData<TtlMap>,
                       static_cast<std::underlying_type_t<DataKind>>(
                           DataKind::kKindCount)>;
        void Set(DataKind kind, std::string const& key, TimePoint expiration);
        void Remove(DataKind kind, std::string const& key);
        std::optional<TimePoint> Get(DataKind kind,
                                     std::string const& key) const;
        void Clear();

        [[nodiscard]] typename DataType::iterator begin();

        [[nodiscard]] typename DataType::iterator end();

       private:
        DataType data_;
    };

    ScopedTtls scoped_;
};

std::ostream& operator<<(std::ostream& out,
                         ExpirationTracker::TrackState const& state);

}  // namespace launchdarkly::server_side::data_components
