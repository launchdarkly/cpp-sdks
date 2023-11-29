#include "lazy_load_system.hpp"

#include "../../data_components/serialization_adapters/json_deserializer.hpp"

namespace launchdarkly::server_side::data_systems {

data_components::FlagKind const LazyLoad::Kinds::Flag =
    data_components::FlagKind();

data_components::SegmentKind const LazyLoad::Kinds::Segment =
    data_components::SegmentKind();

LazyLoad::LazyLoad(Logger const& logger, config::built::LazyLoadConfig cfg)
    : LazyLoad(logger, std::move(cfg), []() {
          return std::chrono::steady_clock::now();
      }) {}

LazyLoad::LazyLoad(Logger const& logger,
                   config::built::LazyLoadConfig cfg,
                   TimeFn time)
    : logger_(logger),
      reader_(std::make_unique<data_components::JsonDeserializer>(cfg.source)),
      time_(std::move(time)),
      fresh_duration_(cfg.refresh_ttl) {}

std::string const& LazyLoad::Identity() const {
    static std::string id = "lazy load via " + reader_->Identity();
    return id;
}

void LazyLoad::Initialize() {}

void LazyLoad::Shutdown() {}

std::shared_ptr<data_model::FlagDescriptor> LazyLoad::GetFlag(
    std::string const& key) const {
    auto const state =
        tracker_.State(data_components::DataKind::kFlag, key, time_());
    return Get<std::shared_ptr<data_model::FlagDescriptor>>(
        state, [this, &key]() { RefreshFlag(key); },
        [this, &key]() { return cache_.GetFlag(key); });
}

std::shared_ptr<data_model::SegmentDescriptor> LazyLoad::GetSegment(
    std::string const& key) const {
    auto const state =
        tracker_.State(data_components::DataKind::kSegment, key, time_());
    return Get<std::shared_ptr<data_model::SegmentDescriptor>>(
        state, [this, &key]() { RefreshSegment(key); },
        [this, &key]() { return cache_.GetSegment(key); });
}

// In the normal course of SDK operation, flags and segments are loaded
// on-demand, are resident in memory for a TTL, and then are refreshed.
// This results in a working set that is eventually consistent. Load on the
// underlying source is spread out relative to the pattern of flag evaluations
// performed by an application.
//
// However, AllFlags is a special case. Here the SDK is asking for all flags
// in order to perform a mass-evaluation for a single context. This could
// theoretically generate thousands of individual refresh calls for both flags
// and segments, which could overwhelm the source.
//
// To optimize this, two calls are made to grab all flags and all segments.
// As long as the TTL is longer than the actual time it takes to evaluate all of
// the flags, no additional calls will need to be made to the source.
//
// To guard against overloading the source when all flags are being constantly
// evaluated, the "all flags" operation itself is assigned a TTL (the same as a
// flag/segment.)
std::unordered_map<std::string, std::shared_ptr<data_model::FlagDescriptor>>
LazyLoad::AllFlags() const {
    auto const state = tracker_.State(Keys::kAllFlags, time_());
    return Get<std::unordered_map<std::string,
                                  std::shared_ptr<data_model::FlagDescriptor>>>(
        state, [this]() { RefreshAllFlags(); },
        [this]() { return cache_.AllFlags(); });
}

std::unordered_map<std::string, std::shared_ptr<data_model::SegmentDescriptor>>
LazyLoad::AllSegments() const {
    auto const state = tracker_.State(Keys::kAllSegments, time_());
    return Get<std::unordered_map<
        std::string, std::shared_ptr<data_model::SegmentDescriptor>>>(
        state, [this]() { RefreshAllSegments(); },
        [this]() { return cache_.AllSegments(); });
}

bool LazyLoad::Initialized() const {
    auto const state = tracker_.State(Keys::kInitialized, time_());
    if (initialized_.has_value()) {
        /* Once initialized, we can always return true. */
        if (initialized_.value()) {
            return true;
        }
        /* If not yet initialized, then we can return false only if the state is
         * fresh - otherwise we should make an attempt to refresh. */
        if (data_components::ExpirationTracker::TrackState::kFresh == state) {
            return false;
        }
    }
    RefreshInitState();
    return initialized_.value_or(false);
}

void LazyLoad::RefreshAllFlags() const {
    RefreshAll(Keys::kAllFlags, data_components::DataKind::kFlag,
               [this]() { return reader_->AllFlags(); });
}

void LazyLoad::RefreshAllSegments() const {
    RefreshAll(Keys::kAllSegments, data_components::DataKind::kSegment,
               [this]() { return reader_->AllSegments(); });
}

void LazyLoad::RefreshInitState() const {
    initialized_ = reader_->Initialized();
    tracker_.Add(Keys::kInitialized, ExpiryTime());
}

void LazyLoad::RefreshSegment(std::string const& segment_key) const {
    RefreshItem(
        data_components::DataKind::kSegment, segment_key,
        [this](std::string const& key) { return reader_->GetSegment(key); },
        [this](std::string const& key) { return cache_.RemoveSegment(key); });
}

void LazyLoad::RefreshFlag(std::string const& flag_key) const {
    RefreshItem(
        data_components::DataKind::kFlag, flag_key,
        [this](std::string const& key) { return reader_->GetFlag(key); },
        [this](std::string const& key) { return cache_.RemoveFlag(key); });
}

std::chrono::time_point<std::chrono::steady_clock> LazyLoad::ExpiryTime()
    const {
    return time_() +
           std::chrono::duration_cast<std::chrono::steady_clock::duration>(
               fresh_duration_);
}

}  // namespace launchdarkly::server_side::data_systems
