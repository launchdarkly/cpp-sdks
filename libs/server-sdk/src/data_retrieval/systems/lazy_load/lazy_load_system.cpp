#include "lazy_load_system.hpp"

#include "../adapters/json_destination.hpp"
#include "../adapters/json_source.hpp"

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

#include "data_version_inspectors.hpp"

namespace launchdarkly::server_side::data_retrieval {

using namespace config::shared::built;

LazyLoad::LazyLoad(
    ServiceEndpoints const& endpoints,
    DataSourceConfig<config::shared::ServerSDK> const& data_source_config,
    HttpProperties http_properties,
    boost::asio::any_io_executor ioc,
    DataSourceStatusManager& status_manager,
    Logger const& logger)
    : store_(), synchronizer_(), bootstrapper_() {}

std::string const& LazyLoad::Identity() const {
    // TODO: Obtain more specific info
    static std::string id = "generic pull-mode source";
}

ISynchronizer* LazyLoad::GetSynchronizer() {
    return reinterpret_cast<ISynchronizer*>(this);
}

IBootstrapper* LazyLoad::GetBootstrapper() {
    // Bootstrapping is not supported in Pull Mode sources yet.
    // It would be simple: perform a get all.
    return nullptr;
}

void LazyLoad::Init(std::optional<data_model::SDKDataSet> initial_data,
                    IDataDestination& destination) {
    // TODO: implement
    // This would happen if we bootstrapped from some source, and now that
    // data is being passed here for management.
    // The data would need to be sorted before passing furthur.
}
void LazyLoad::Start() {
    // No-op, because we all data is pulled on demand.
}
void LazyLoad::ShutdownAsync(std::function<void()>) {
    // Similar to Start, also a no-op since there's nothing to shutdown
    // (perhaps explicitly close database client?)
}

std::shared_ptr<FlagDescriptor> LazyLoad::GetFlag(
    std::string const& key) const {
    auto state = tracker_.State(Keys::kAllSegments, time_());
    return Get<std::shared_ptr<FlagDescriptor>>(
        state, [this, &key]() { RefreshFlag(key); },
        [this, &key]() { return memory_store_.GetFlag(key); });
}

std::shared_ptr<SegmentDescriptor> LazyLoad::GetSegment(
    std::string const& key) const {
    auto state = tracker_.State(Keys::kAllSegments, time_());
    return Get<std::shared_ptr<SegmentDescriptor>>(
        state, [this, &key]() { RefreshSegment(key); },
        [this, &key]() { return memory_store_.GetSegment(key); });
}

std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>>
LazyLoad::AllFlags() const {
    auto state = tracker_.State(Keys::kAllFlags, time_());
    return Get<
        std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>>>(
        state, [this]() { RefreshAllFlags(); },
        [this]() { return memory_store_.AllFlags(); });
}

std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>
LazyLoad::AllSegments() const {
    auto state = tracker_.State(Keys::kAllSegments, time_());
    return Get<
        std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>>(
        state, [this]() { RefreshAllSegments(); },
        [this]() { return memory_store_.AllSegments(); });
}

PersistentStore::FlagKind const PersistentStore::Kinds::Flag = FlagKind();
PersistentStore::SegmentKind const PersistentStore::Kinds::Segment =
    SegmentKind();

bool PersistentStore::Initialized() const {
    auto state = tracker_.State(Keys::kInitialized, time_());
    if (initialized_.has_value()) {
        if (initialized_.value()) {
            return true;
        }
        if (ExpirationTracker::TrackState::kFresh == state) {
            return initialized_.value();
        }
    }
    RefreshInitState();
    return initialized_.value_or(false);
}

void LazyLoad::RefreshAllFlags() const {
    auto res = core_->All(Kinds::Flag);
    // TODO: Deserialize and put in store.
    tracker_.Add(Keys::kAllSegments, time_());
}

void LazyLoad::RefreshAllSegments() const {
    auto res = core_->All(Kinds::Segment);
    // TODO: Deserialize and put in store.
    tracker_.Add(Keys::kAllFlags, time_());
}

void LazyLoad::RefreshInitState() const {
    initialized_ = core_->Initialized();
    tracker_.Add(Keys::kInitialized, time_());
}

void LazyLoad::RefreshSegment(std::string const& key) const {
    auto res = core_->Get(Kinds::Segment, key);
    if (res.has_value()) {
        if (res->has_value()) {
            auto segment = DeserializeSegment(res->value());
            if (segment.has_value()) {
                memory_store_.Upsert(key, segment.value());
            }
            // TODO: Log that we got bogus data?
        }
        tracker_.Add(DataKind::kSegment, key, time_());
    }
    // TODO: If there is an actual error, then do we not reset the tracking?
}

void LazyLoad::RefreshFlag(std::string const& key) const {
    auto res = core_->Get(Kinds::Segment, key);
    if (res.has_value()) {
        if (res->has_value()) {
            auto flag = DeserializeFlag(res->value());
            if (flag.has_value()) {
                memory_store_.Upsert(key, flag.value());
            }
            // TODO: Log that we got bogus data?
        }
        tracker_.Add(DataKind::kSegment, key, time_());
    }
    // TODO: If there is an actual error, then do we not reset the tracking?
}

}  // namespace launchdarkly::server_side::data_retrieval
