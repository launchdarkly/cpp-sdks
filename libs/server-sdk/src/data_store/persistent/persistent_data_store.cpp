#include "persistent_data_store.hpp"

namespace launchdarkly::server_side::data_store::persistent {
PersistentStore::PersistentStore(
    std::shared_ptr<persistence::IPersistentStoreCore> core,
    std::chrono::seconds cache_refresh_time,
    std::optional<std::chrono::seconds> eviction_interval,
    std::function<std::chrono::time_point<std::chrono::steady_clock>()> time)
    : core_(core), time_(time) {}

std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>>
PersistentStore::AllFlags() const {
    auto state = tracker_.State(Keys::kAllFlags, time_());
    return Get<
        std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>>>(
        state, [this]() { RefreshAllFlags(); },
        [this]() { return memory_store_.AllFlags(); });
}

std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>
PersistentStore::AllSegments() const {
    auto state = tracker_.State(Keys::kAllSegments, time_());
    return Get<
        std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>>(
        state, [this]() { RefreshAllSegments(); },
        [this]() { return memory_store_.AllSegments(); });
}

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

std::string const& PersistentStore::Description() const {
    return core_->Description();
}
void PersistentStore::Init(launchdarkly::data_model::SDKDataSet dataSet) {
    // TODO: Implement sort.
    // TODO: Serialize the items.
}
void PersistentStore::Upsert(std::string const& key,
                             SegmentDescriptor segment) {
    // TODO: Serialize the item.
}

void PersistentStore::Upsert(std::string const& key, FlagDescriptor flag) {
    // TODO: Serialize the item.
}

std::shared_ptr<SegmentDescriptor> PersistentStore::GetSegment(
    std::string const& key) const {
    auto state = tracker_.State(Keys::kAllSegments, time_());
    return Get<std::shared_ptr<SegmentDescriptor>>(
        state, [this, &key]() { RefreshSegment(key); },
        [this, &key]() { return memory_store_.GetSegment(key); });
}

std::shared_ptr<FlagDescriptor> PersistentStore::GetFlag(
    std::string const& key) const {
    auto state = tracker_.State(Keys::kAllSegments, time_());
    return Get<std::shared_ptr<FlagDescriptor>>(
        state, [this, &key]() { RefreshFlag(key); },
        [this, &key]() { return memory_store_.GetFlag(key); });
}
void PersistentStore::RefreshAllFlags() const {
    auto res = core_->All(Kinds::Flag);
    // TODO: Deserialize and put in store.
    tracker_.Add(Keys::kAllSegments, time_());
}

void PersistentStore::RefreshAllSegments() const {
    auto res = core_->All(Kinds::Segment);
    // TODO: Deserialize and put in store.
    tracker_.Add(Keys::kAllFlags, time_());
}

void PersistentStore::RefreshInitState() const {
    initialized_ = core_->Initialized();
    tracker_.Add(Keys::kInitialized, time_());
}

void PersistentStore::RefreshSegment(std::string const& key) const {
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

void PersistentStore::RefreshFlag(std::string const& key) const {
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

std::optional<FlagDescriptor> PersistentStore::DeserializeFlag(
    persistence::SerializedItemDescriptor flag) {
    // TODO: Implement
    return launchdarkly::server_side::data_store::FlagDescriptor(0);
}

std::optional<SegmentDescriptor> PersistentStore::DeserializeSegment(
    persistence::SerializedItemDescriptor segment) {
    // TODO: Implement
    return launchdarkly::server_side::data_store::SegmentDescriptor(0);
}

std::string const& PersistentStore::SegmentKind::Namespace() const {
    return namespace_;
}

uint64_t PersistentStore::SegmentKind::Version(std::string const& data) const {
    // TODO: Deserialize.
    return 0;
}

std::string const& PersistentStore::FlagKind::Namespace() const {
    return namespace_;
}

uint64_t PersistentStore::FlagKind::Version(std::string const& data) const {
    // TODO: Deserialize.
    return 0;
}

}  // namespace launchdarkly::server_side::data_store::persistent
