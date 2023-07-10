#include "data_store_updater.hpp"

#include <launchdarkly/signals/boost_signal_connection.hpp>

namespace launchdarkly::server_side::data_store {

std::unique_ptr<IConnection> DataStoreUpdater::OnFlagChange(
    launchdarkly::server_side::IChangeNotifier::ChangeHandler handler) {
    std::lock_guard lock{signal_mutex_};

    return std::make_unique<launchdarkly::internal::signals::SignalConnection>(
        signals_.connect(handler));
}

void DataStoreUpdater::Init(launchdarkly::data_model::SDKDataSet dataSet) {
    std::optional<DependencySet> changeNotifications;
    if (HasListeners()) {
        auto updatedItems = DependencySet();

        CalculateChanges(DataKind::kFlag, store_->AllFlags(), dataSet.flags,
                         updatedItems);
        CalculateChanges(DataKind::kSegment, store_->AllSegments(),
                         dataSet.segments, updatedItems);
        changeNotifications = updatedItems;
    }

    dependencyTracker_.Clear();
    for (auto const& flag : dataSet.flags) {
        dependencyTracker_.UpdateDependencies(flag.first, flag.second);
    }
    for (auto const& segment : dataSet.segments) {
        dependencyTracker_.UpdateDependencies(segment.first, segment.second);
    }
    // Data will move into the store, so we want to update dependencies before
    // it is moved.
    sink_->Init(dataSet);
    // After updating the sunk let listeners know of changes.
    if (changeNotifications) {
        NotifyChanges(*changeNotifications);
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
    signals_(std::make_shared<ChangeSet>(
        std::move(changes.SetForKind(DataKind::kFlag))));
}

}  // namespace launchdarkly::server_side::data_store
