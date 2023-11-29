// The Lazy Load system is responsible for loading flags and segments from
// an underlying source when requested by the SDK's evaluation algorithm.
//
// This is fundamentally different than Background Sync, where all items
// are loaded into memory at initialization, and then updated asynchronously
// when changes arrive from LaunchDarkly.

// In this system, items are updated through a cache refresh process which
// depends on a TTL.
//
// This TTL is configured by the user, and represents a tradeoff between
// freshness (and consistency), and load/traffic to the underyling source.
//
// In the normal course of SDK operation, individual flags and segments loaded
// over time depending on the patterns of SDK usage present in an application.
// This generally spreads out the load on the source over time.
//
// A different usage pattern is caused by the SDK's "AllFlags" API, which
// evaluates all flags for a given context. Because this usage is potentially
// extremely costly (if there are thousands of flags or segments to be loaded),
// the Lazy Load system optimizes by performing a special "GetAll" operation
// on the underlying source. This operation fetches the entire set of a data
// kind (either flags or segments) in one trip, and then stores them in the
// cache.
//
// It's important that the "GetAll" operation also be bound by a TTL, so that
// repeated calls to "AllFlags" don't cause repeated calls to the source. The
// TTL for this operation is identical to the indivudal-item TTL configurable by
// the user.
//
// An implication of the current implementation is that if Lazy Load is being
// used to handle a "sparse" environment - that is, the environment is too big
// to load into memory, and so loading on demand is desirable - calling
// "AllFlags" will destroy that property because items are not actively evicted
// from the cache. On the other hand, that property could be used to prime the
// SDK's memory cache, preventing the need to individually load flags or
// segments.
//
// The current design does not perform active eviction when an item is stale
// because it is generally better to serve stale data than none at all. If the
// source is unavailable, the SDK will be able to indefinitely serve the last
// known values. Active eviction can be added in the future as a configurable
// option.

#include "lazy_load_system.hpp"

#include "../../data_components/serialization_adapters/json_deserializer.hpp"

namespace launchdarkly::server_side::data_systems {

data_components::FlagKind const LazyLoad::Kinds::Flag =
    data_components::FlagKind();

data_components::SegmentKind const LazyLoad::Kinds::Segment =
    data_components::SegmentKind();

LazyLoad::LazyLoad(Logger const& logger,
                   config::built::LazyLoadConfig cfg,
                   data_components::DataSourceStatusManager& status_manager)
    : LazyLoad(logger, std::move(cfg), status_manager, []() {
          return std::chrono::steady_clock::now();
      }) {}

LazyLoad::LazyLoad(Logger const& logger,
                   config::built::LazyLoadConfig cfg,
                   data_components::DataSourceStatusManager& status_manager,
                   TimeFn time)
    : logger_(logger),
      reader_(std::make_unique<data_components::JsonDeserializer>(
          logger,
          std::move(cfg.source))),
      status_manager_(status_manager),
      time_(std::move(time)),
      fresh_duration_(cfg.refresh_ttl) {}

std::string const& LazyLoad::Identity() const {
    static std::string id = "lazy load via " + reader_->Identity();
    return id;
}

void LazyLoad::Initialize() {
    status_manager_.SetState(DataSourceState::kInitializing);
    if (Initialized()) {
        status_manager_.SetState(DataSourceState::kValid);
    }
}

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
    /* Since the memory store isn't provisioned with an initial SDKDataSet
     * like in the Background Sync system, we can't forward this call to
     * MemoryStore::Initialized(). Instead, we need to check the state of the
     * underlying source. */

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
