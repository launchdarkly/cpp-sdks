#include "expiration_tracker.hpp"

namespace launchdarkly::server_side::data_store::persistent {

void ExpirationTracker::Add(std::string const& key,
                            ExpirationTracker::TimePoint expiration) {}
void ExpirationTracker::Remove(std::string const& key) {}
ExpirationTracker::TrackState ExpirationTracker::State(
    std::string const& key,
    ExpirationTracker::TimePoint current_time) {
    return ExpirationTracker::TrackState::kFresh;
}
void ExpirationTracker::Add(data_store::DataKind kind,
                            std::string const& key,
                            ExpirationTracker::TimePoint expiration) {}
void ExpirationTracker::Remove(data_store::DataKind kind,
                               std::string const& key) {}
ExpirationTracker::TrackState ExpirationTracker::State(
    data_store::DataKind kind,
    std::string const& key,
    ExpirationTracker::TimePoint current_time) {
    return ExpirationTracker::TrackState::kFresh;
}
void ExpirationTracker::Clear() {}
void ExpirationTracker::ScopedTtls::Set(
    DataKind kind,
    std::string const& key,
    ExpirationTracker::TimePoint expiration) {}
void ExpirationTracker::ScopedTtls::Remove(DataKind kind, std::string) {}
void ExpirationTracker::ScopedTtls::Clear() {}
}  // namespace launchdarkly::server_side::data_store::persistent
