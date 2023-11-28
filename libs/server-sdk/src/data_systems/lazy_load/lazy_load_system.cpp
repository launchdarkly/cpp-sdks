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
        state,
        [this]() {
            RefreshAllFlags();
            RefreshAllSegments();
        },
        [this]() {
            return cache_
                .AllFlags();  // segments will be accessed as-needed by the
            // evaluation algorithm
        });
}

std::unordered_map<std::string, std::shared_ptr<data_model::SegmentDescriptor>>
LazyLoad::AllSegments() const {
    return cache_.AllSegments();
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
    auto const updated_expiry = ExpiryTime();
    tracker_.Add(Keys::kAllFlags, updated_expiry);

    if (auto all_flags = reader_->AllFlags()) {
        for (auto flag : *all_flags) {
            cache_.Upsert(flag.first, std::move(flag.second));
            tracker_.Add(data_components::DataKind::kFlag, flag.first,
                         updated_expiry);
        }
    } else {
        LD_LOG(logger_, LogLevel::kError)
            << "failed to refresh all flags via " << reader_->Identity() << ": "
            << all_flags.error();
    }
}

void LazyLoad::RefreshAllSegments() const {
    auto const updated_expiry = ExpiryTime();
    if (auto all_segments = reader_->AllSegments()) {
        for (auto segment : *all_segments) {
            cache_.Upsert(segment.first, std::move(segment.second));
            tracker_.Add(data_components::DataKind::kSegment, segment.first,
                         updated_expiry);
        }
    } else {
        LD_LOG(logger_, LogLevel::kError)
            << "failed to refresh all segments via " << reader_->Identity()
            << ": " << all_segments.error();
    }
}

void LazyLoad::RefreshInitState() const {
    initialized_ = reader_->Initialized();
    tracker_.Add(Keys::kInitialized, ExpiryTime());
}

void LazyLoad::RefreshSegment(std::string const& key) const {
    // Rate limit in all cases to protect the underlying store.
    tracker_.Add(data_components::DataKind::kSegment, key, ExpiryTime());

    if (auto segment_result = reader_->GetSegment(key)) {
        if (auto optional_segment = *segment_result) {
            cache_.Upsert(key, std::move(*optional_segment));
        } else {
            LD_LOG(logger_, LogLevel::kDebug)
                << "segment " << key << " requested but not found via "
                << reader_->Identity();
            cache_.RemoveSegment(key);
        }
    } else {
        LD_LOG(logger_, LogLevel::kError)
            << "failed to refresh segment " << key << " via "
            << reader_->Identity() << ": " << segment_result.error();
    }
}

void LazyLoad::RefreshFlag(std::string const& key) const {
    // Rate limit in all cases to protect the underlying store.
    tracker_.Add(data_components::DataKind::kFlag, key, ExpiryTime());

    if (auto flag_result = reader_->GetFlag(key)) {
        if (auto optional_flag = *flag_result) {
            cache_.Upsert(key, std::move(*optional_flag));
        } else {
            LD_LOG(logger_, LogLevel::kDebug)
                << "flag " << key << " requested but not found via "
                << reader_->Identity();
            cache_.RemoveFlag(key);
        }
    } else {
        LD_LOG(logger_, LogLevel::kError)
            << "failed to refresh flag " << key << " via "
            << reader_->Identity() << ": " << flag_result.error();
    }
}

std::chrono::time_point<std::chrono::steady_clock> LazyLoad::ExpiryTime()
    const {
    return time_() +
           std::chrono::duration_cast<std::chrono::steady_clock::duration>(
               fresh_duration_);
}

}  // namespace launchdarkly::server_side::data_systems
