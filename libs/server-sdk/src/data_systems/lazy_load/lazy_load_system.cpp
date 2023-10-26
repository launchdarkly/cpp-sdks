#include "lazy_load_system.hpp"

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

namespace launchdarkly::server_side::data_systems {

using namespace config::shared::built;

LazyLoad::LazyLoad(
    ServiceEndpoints const& endpoints,
    DataSourceConfig<config::shared::ServerSDK> const& data_source_config,
    HttpProperties http_properties,
    boost::asio::any_io_executor ioc,
    data_components::DataSourceStatusManager& status_manager,
    Logger const& logger)
    : cache_() {}

std::string const& LazyLoad::Identity() const {
    // TODO: Obtain more specific info
    static std::string id = "generic lazy-loader";
    return id;
}

void LazyLoad::Initialize() {}

std::shared_ptr<data_model::FlagDescriptor> LazyLoad::GetFlag(
    std::string const& key) const {
    auto state = tracker_.State(Keys::kAllSegments, time_());
    return Get<std::shared_ptr<data_model::FlagDescriptor>>(
        state, [this, &key]() { RefreshFlag(key); },
        [this, &key]() { return cache_.GetFlag(key); });
}

std::shared_ptr<data_model::SegmentDescriptor> LazyLoad::GetSegment(
    std::string const& key) const {
    auto state = tracker_.State(Keys::kAllSegments, time_());
    return Get<std::shared_ptr<data_model::SegmentDescriptor>>(
        state, [this, &key]() { RefreshSegment(key); },
        [this, &key]() { return cache_.GetSegment(key); });
}

std::unordered_map<std::string, std::shared_ptr<data_model::FlagDescriptor>>
LazyLoad::AllFlags() const {
    auto state = tracker_.State(Keys::kAllFlags, time_());
    return Get<std::unordered_map<std::string,
                                  std::shared_ptr<data_model::FlagDescriptor>>>(
        state, [this]() { RefreshAllFlags(); },
        [this]() { return cache_.AllFlags(); });
}

std::unordered_map<std::string, std::shared_ptr<data_model::SegmentDescriptor>>
LazyLoad::AllSegments() const {
    auto state = tracker_.State(Keys::kAllSegments, time_());
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
    auto state = tracker_.State(Keys::kInitialized, time_());
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
    auto res = source_->All(Kinds::Flag);
    // TODO: Deserialize and put in store.
    tracker_.Add(Keys::kAllSegments, time_());
}

void LazyLoad::RefreshAllSegments() const {
    auto res = source_->All(Kinds::Segment);
    // TODO: Deserialize and put in store.
    tracker_.Add(Keys::kAllFlags, time_());
}

void LazyLoad::RefreshInitState() const {
    initialized_ = source_->Initialized();
    tracker_.Add(Keys::kInitialized, time_());
}

void LazyLoad::RefreshSegment(std::string const& key) const {
    auto res = source_->Get(Kinds::Segment, key);
    if (res.has_value()) {
        if (res->has_value()) {
            auto segment = DeserializeSegment(res->value());
            if (segment.has_value()) {
                cache_.Upsert(key, segment.value());
            }
            // TODO: Log that we got bogus data?
        }
        tracker_.Add(data_components::DataKind::kSegment, key, time_());
    }
    // TODO: If there is an actual error, then do we not reset the tracking?
}

void LazyLoad::RefreshFlag(std::string const& key) const {
    auto res = source_->Get(Kinds::Segment, key);
    if (res.has_value()) {
        if (res->has_value()) {
            auto flag = DeserializeFlag(res->value());
            if (flag.has_value()) {
                cache_.Upsert(key, flag.value());
            }
            // TODO: Log that we got bogus data?
        }
        tracker_.Add(data_components::DataKind::kSegment, key, time_());
    }
    // TODO: If there is an actual error, then do we not reset the tracking?
}

}  // namespace launchdarkly::server_side::data_systems