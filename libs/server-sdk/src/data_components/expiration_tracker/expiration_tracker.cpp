#include "expiration_tracker.hpp"

#include <ostream>

namespace launchdarkly::server_side::data_components {

void ExpirationTracker::Add(std::string const& key, TimePoint expiration) {
    unscoped_.insert_or_assign(key, expiration);
}

void ExpirationTracker::Remove(std::string const& key) {
    unscoped_.erase(key);
}

ExpirationTracker::TrackState ExpirationTracker::State(
    std::string const& key,
    TimePoint current_time) const {
    auto item = unscoped_.find(key);
    if (item != unscoped_.end()) {
        return State(item->second, current_time);
    }

    return TrackState::kNotTracked;
}

void ExpirationTracker::Add(DataKind kind,
                            std::string const& key,
                            TimePoint expiration) {
    scoped_.Set(kind, key, expiration);
}

void ExpirationTracker::Remove(DataKind kind, std::string const& key) {
    scoped_.Remove(kind, key);
}

ExpirationTracker::TrackState ExpirationTracker::State(
    DataKind kind,
    std::string const& key,
    TimePoint current_time) const {
    auto expiration = scoped_.Get(kind, key);
    if (expiration.has_value()) {
        return State(expiration.value(), current_time);
    }
    return TrackState::kNotTracked;
}

void ExpirationTracker::Clear() {
    scoped_.Clear();
    unscoped_.clear();
}
std::vector<std::pair<std::optional<DataKind>, std::string>>
ExpirationTracker::Prune(TimePoint current_time) {
    std::vector<std::pair<std::optional<DataKind>, std::string>> pruned;

    // Determine everything to be pruned.
    for (auto const& item : unscoped_) {
        if (State(item.second, current_time) == TrackState::kStale) {
            pruned.emplace_back(std::nullopt, item.first);
        }
    }
    for (auto const& scope : scoped_) {
        for (auto const& item : scope.Data()) {
            if (State(item.second, current_time) == TrackState::kStale) {
                pruned.emplace_back(scope.Kind(), item.first);
            }
        }
    }

    // Do the actual prune.
    for (auto const& item : pruned) {
        if (item.first.has_value()) {
            scoped_.Remove(item.first.value(), item.second);
        } else {
            unscoped_.erase(item.second);
        }
    }
    return pruned;
}
ExpirationTracker::TrackState ExpirationTracker::State(TimePoint expiration,
                                                       TimePoint current_time) {
    if (expiration > current_time) {
        return TrackState::kFresh;
    }
    return TrackState::kStale;
}

void ExpirationTracker::ScopedTtls::Set(DataKind kind,
                                        std::string const& key,
                                        TimePoint expiration) {
    data_[static_cast<std::underlying_type_t<DataKind>>(kind)]
        .Data()
        .insert_or_assign(key, expiration);
}

void ExpirationTracker::ScopedTtls::Remove(DataKind kind,
                                           std::string const& key) {
    data_[static_cast<std::underlying_type_t<DataKind>>(kind)].Data().erase(
        key);
}

void ExpirationTracker::ScopedTtls::Clear() {
    for (auto& scope : data_) {
        scope.Data().clear();
    }
}

std::optional<ExpirationTracker::TimePoint> ExpirationTracker::ScopedTtls::Get(
    DataKind kind,
    std::string const& key) const {
    auto const& scope =
        data_[static_cast<std::underlying_type_t<DataKind>>(kind)];
    auto found = scope.Data().find(key);
    if (found != scope.Data().end()) {
        return found->second;
    }
    return std::nullopt;
}
ExpirationTracker::ScopedTtls::ScopedTtls()
    : data_{
          TaggedData<TtlMap>(DataKind::kFlag),
          TaggedData<TtlMap>(DataKind::kSegment),
      } {}

std::array<TaggedData<ExpirationTracker::TtlMap>, 2>::iterator
ExpirationTracker::ScopedTtls::begin() {
    return data_.begin();
}

std::array<TaggedData<ExpirationTracker::TtlMap>, 2>::iterator
ExpirationTracker::ScopedTtls::end() {
    return data_.end();
}

std::ostream& operator<<(std::ostream& out,
                         ExpirationTracker::TrackState const& state) {
    switch (state) {
        case ExpirationTracker::TrackState::kFresh:
            out << "FRESH";
            break;
        case ExpirationTracker::TrackState::kStale:
            out << "STALE";
            break;
        case ExpirationTracker::TrackState::kNotTracked:
            out << "NOT_TRACKED";
            break;
    }
    return out;
}
}  // namespace launchdarkly::server_side::data_components
