#include "pull_mode_data_source.hpp"

#include "../polling_data_source.hpp"
#include "../streaming_data_source.hpp"

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

#include "data_version_inspectors.hpp"

namespace launchdarkly::server_side::data_sources {

using namespace config::shared::built;

PullModeSource::PullModeSource(
    ServiceEndpoints const& endpoints,
    DataSourceConfig<config::shared::ServerSDK> const& data_source_config,
    HttpProperties http_properties,
    boost::asio::any_io_executor ioc,
    DataSourceStatusManager& status_manager,
    Logger const& logger)
    : store_(), synchronizer_(), bootstrapper_() {}

std::string PullModeSource::Identity() const {
    // TODO: Obtain more specific info
    return "generic pull-mode source";
}

ISynchronizer* PullModeSource::GetSynchronizer() {
    return reinterpret_cast<ISynchronizer*>(this);
}

IBootstrapper* PullModeSource::GetBootstrapper() {
    // Bootstrapping is not supported in Pull Mode sources yet.
    // It would be simple: perform a get all.
    return nullptr;
}

void PullModeSource::Init(std::optional<data_model::SDKDataSet> initial_data,
                          IDataDestination& destination) {
    // TODO: implement
    // This would happen if we bootstrapped from some source, and now that
    // data is being passed here for management.
    // The data would need to be sorted before passing furthur.
}
void PullModeSource::Start() {
    // No-op, because we all data is pulled on demand.
}
void PullModeSource::ShutdownAsync(std::function<void()>) {
    // Similar to Start, also a no-op since there's nothing to shutdown
    // (perhaps explicitly close database client?)
}

std::shared_ptr<FlagDescriptor> PullModeSource::GetFlag(
    std::string const& key) const {
    auto state = tracker_.State(Keys::kAllSegments, time_());
    return Get<std::shared_ptr<FlagDescriptor>>(
        state, [this, &key]() { RefreshFlag(key); },
        [this, &key]() { return memory_store_.GetFlag(key); });
}

std::shared_ptr<SegmentDescriptor> PullModeSource::GetSegment(
    std::string const& key) const {
    auto state = tracker_.State(Keys::kAllSegments, time_());
    return Get<std::shared_ptr<SegmentDescriptor>>(
        state, [this, &key]() { RefreshSegment(key); },
        [this, &key]() { return memory_store_.GetSegment(key); });
}

std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>>
PullModeSource::AllFlags() const {
    auto state = tracker_.State(Keys::kAllFlags, time_());
    return Get<
        std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>>>(
        state, [this]() { RefreshAllFlags(); },
        [this]() { return memory_store_.AllFlags(); });
}

std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>
PullModeSource::AllSegments() const {
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

void PullModeSource::RefreshAllFlags() const {
    auto res = core_->All(Kinds::Flag);
    // TODO: Deserialize and put in store.
    tracker_.Add(Keys::kAllSegments, time_());
}

void PullModeSource::RefreshAllSegments() const {
    auto res = core_->All(Kinds::Segment);
    // TODO: Deserialize and put in store.
    tracker_.Add(Keys::kAllFlags, time_());
}

void PullModeSource::RefreshInitState() const {
    initialized_ = core_->Initialized();
    tracker_.Add(Keys::kInitialized, time_());
}

void PullModeSource::RefreshSegment(std::string const& key) const {
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

void PullModeSource::RefreshFlag(std::string const& key) const {
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

// TODO: Move this to the JSONSerializationAdapter
persistence::SerializedItemDescriptor PersistentStore::Serialize(
    FlagDescriptor flag) {
    // TODO: Implement
    return persistence::SerializedItemDescriptor();
}

persistence::SerializedItemDescriptor PersistentStore::Serialize(
    SegmentDescriptor segment) {
    // TODO: Implement
    return persistence::SerializedItemDescriptor();
}

template <typename TData>
static std::optional<data_model::ItemDescriptor<TData>> Deserialize(
    persistence::SerializedItemDescriptor item) {
    if (item.deleted) {
        return data_model::ItemDescriptor<TData>(item.version);
    }

    boost::json::error_code error_code;
    if (!item.serializedItem.has_value()) {
        return std::nullopt;
    }
    auto parsed = boost::json::parse(item.serializedItem.value(), error_code);

    if (error_code) {
        return std::nullopt;
    }

    auto res =
        boost::json::value_to<tl::expected<std::optional<TData>, JsonError>>(
            parsed);

    if (res.has_value() && res->has_value()) {
        return data_model::ItemDescriptor(res->value());
    }

    return std::nullopt;
}

std::optional<FlagDescriptor> PersistentStore::DeserializeFlag(
    persistence::SerializedItemDescriptor flag) {
    return Deserialize<data_model::Flag>(flag);
}

std::optional<SegmentDescriptor> PersistentStore::DeserializeSegment(
    persistence::SerializedItemDescriptor segment) {
    return Deserialize<data_model::Segment>(segment);
}

template <typename TData>
static uint64_t GetVersion(std::string data) {
    boost::json::error_code error_code;
    auto parsed = boost::json::parse(data, error_code);

    if (error_code) {
        return 0;
    }
    auto res =
        boost::json::value_to<tl::expected<std::optional<TData>, JsonError>>(
            parsed);

    if (res.has_value() && res->has_value()) {
        return res->value().version;
    }
    return 0;
}

std::string const& PersistentStore::SegmentKind::Namespace() const {
    return namespace_;
}

uint64_t PersistentStore::SegmentKind::Version(std::string const& data) const {
    return GetVersion<data_model::Segment>(data);
}

std::string const& PersistentStore::FlagKind::Namespace() const {
    return namespace_;
}

uint64_t PersistentStore::FlagKind::Version(std::string const& data) const {
    return GetVersion<data_model::Flag>(data);
}

}  // namespace launchdarkly::server_side::data_sources
