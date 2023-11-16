#include "lazy_load_system.hpp"

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

#include "sources/redis/redis_source.hpp"

namespace launchdarkly::server_side::data_systems {

using namespace config::shared::built;

/*
*DataSourceConfig<config::shared::ServerSDK> const& data_source_config,
HttpProperties http_properties,
boost::asio::any_io_executor ioc,
data_components::DataSourceStatusManager& status_manager,
*/
LazyLoad::LazyLoad()
    : cache_(),
      raw_source_(
          std::make_shared<RedisDataSource>("tcp://localhost:6379", "test")),
      source_(*raw_source_.get()),
      tracker_(),
      time_([]() { return std::chrono::steady_clock::now(); }),
      initialized_() {}

std::string const& LazyLoad::Identity() const {
    static std::string id = "lazy load via " + source_.Identity();
    return id;
}

void LazyLoad::Initialize() {}

std::shared_ptr<data_model::FlagDescriptor> LazyLoad::GetFlag(
    std::string const& key) const {
    auto const state = tracker_.State(Keys::kAllSegments, time_());
    return Get<std::shared_ptr<data_model::FlagDescriptor>>(
        state, [this, &key]() { RefreshFlag(key); },
        [this, &key]() { return cache_.GetFlag(key); });
}

std::shared_ptr<data_model::SegmentDescriptor> LazyLoad::GetSegment(
    std::string const& key) const {
    auto const state = tracker_.State(Keys::kAllSegments, time_());
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

data_components::FlagKind const LazyLoad::Kinds::Flag =
    data_components::FlagKind();

data_components::SegmentKind const LazyLoad::Kinds::Segment =
    data_components::SegmentKind();

bool LazyLoad::Initialized() const {
    auto const state = tracker_.State(Keys::kInitialized, time_());
    if (initialized_.has_value()) {
        if (initialized_.value()) {
            return true;
        }
        if (data_components::ExpirationTracker::TrackState::kFresh == state) {
            return initialized_.value();
        }
    }
    RefreshInitState();
    return initialized_.value_or(false);
}

void LazyLoad::RefreshAllFlags() const {
    for (auto const [flag_key, flag_descriptor] : source_.AllFlags()) {
        cache_.Upsert(flag_key, std::move(flag_descriptor));
    }
    tracker_.Add(Keys::kAllFlags, time_());
}

void LazyLoad::RefreshAllSegments() const {
    for (auto const [seg_key, seg_descriptor] : source_.AllSegments()) {
        cache_.Upsert(seg_key, std::move(seg_descriptor));
    }
    tracker_.Add(Keys::kAllSegments, time_());
}

void LazyLoad::RefreshInitState() const {
    initialized_ = source_.Initialized();
    tracker_.Add(Keys::kInitialized, time_());
}

void LazyLoad::RefreshSegment(std::string const& key) const {
    if (auto segment_result = source_.GetSegment(key)) {
        if (auto optional_segment = *segment_result) {
            cache_.Upsert(key, std::move(*optional_segment));
        }
        tracker_.Add(data_components::DataKind::kSegment, key, time_());
    }
    // TODO: If there is an actual error, then do we not reset the tracking?
}

void LazyLoad::RefreshFlag(std::string const& key) const {
    if (auto flag_result = source_.GetFlag(key)) {
        if (auto optional_flag = *flag_result) {
            cache_.Upsert(key, std::move(*optional_flag));
        }
        tracker_.Add(data_components::DataKind::kFlag, key, time_());
    }
    // TODO: If there is an actual error, then do we not reset the tracking?
}

}  // namespace launchdarkly::server_side::data_systems