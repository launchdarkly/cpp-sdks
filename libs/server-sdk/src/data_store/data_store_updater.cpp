#include "data_store_updater.hpp"

#include <launchdarkly/signals/boost_signal_connection.hpp>
#include <utility>

namespace launchdarkly::server_side::data_store {

std::unique_ptr<IConnection> DataStoreUpdater::OnFlagChange(
    launchdarkly::server_side::IChangeNotifier::ChangeHandler handler) {
    std::lock_guard lock{signal_mutex_};

    return std::make_unique<launchdarkly::internal::signals::SignalConnection>(
        signals_.connect(handler));
}

void DataStoreUpdater::Init(launchdarkly::data_model::SDKDataSet data_set) {
    std::optional<DependencySet> change_notifications;
    if (HasListeners()) {
        auto updated_items = DependencySet();

        CalculateChanges(DataKind::kFlag, store_->AllFlags(), data_set.flags,
                         updated_items);
        CalculateChanges(DataKind::kSegment, store_->AllSegments(),
                         data_set.segments, updated_items);
        change_notifications = updated_items;
    }

    dependency_tracker_.Clear();
    for (auto const& flag : data_set.flags) {
        dependency_tracker_.UpdateDependencies(flag.first, flag.second);
    }
    for (auto const& segment : data_set.segments) {
        dependency_tracker_.UpdateDependencies(segment.first, segment.second);
    }
    // Data will move into the store, so we want to update dependencies before
    // it is moved.
    sink_->Init(data_set);
    // After updating the sunk let listeners know of changes.
    if (change_notifications) {
        NotifyChanges(*change_notifications);
    }
}

void DataStoreUpdater::Upsert(std::string key,
                              launchdarkly::server_side::data_source::
                                  IDataSourceUpdateSink::FlagDescriptor flag) {
    UpsertCommon(DataKind::kFlag, key, store_->GetFlag(key), flag);
}

void DataStoreUpdater::Upsert(
    std::string key,
    launchdarkly::server_side::data_source::IDataSourceUpdateSink::
        SegmentDescriptor segment) {
    UpsertCommon(DataKind::kSegment, key, store_->GetSegment(key), segment);
}

bool DataStoreUpdater::HasListeners() const {
    std::lock_guard lock{signal_mutex_};
    return !signals_.empty();
}

void DataStoreUpdater::NotifyChanges(DependencySet changes) {
    std::lock_guard lock{signal_mutex_};
    auto flag_changes = changes.SetForKind(DataKind::kFlag);
    // Only emit an event if there are changes.
    if (!flag_changes.empty()) {
        signals_(std::make_shared<ChangeSet>(std::move(flag_changes)));
    }
}

DataStoreUpdater::DataStoreUpdater(std::shared_ptr<IDataSourceUpdateSink> sink,
                                   std::shared_ptr<IDataStore> store)
    : sink_(std::move(sink)), store_(std::move(store)) {}

}  // namespace launchdarkly::server_side::data_store
