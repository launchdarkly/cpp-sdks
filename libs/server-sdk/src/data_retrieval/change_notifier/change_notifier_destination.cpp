#include "change_notifier_destination.hpp"

#include <launchdarkly/signals/boost_signal_connection.hpp>
#include <utility>

namespace launchdarkly::server_side::data_retrieval {

std::unique_ptr<IConnection> DataStoreUpdater::OnFlagChange(
    launchdarkly::server_side::IChangeNotifier::ChangeHandler handler) {
    std::lock_guard lock{signal_mutex_};

    return std::make_unique<launchdarkly::internal::signals::SignalConnection>(
        signals_.connect(handler));
}

void DataStoreUpdater::Init(launchdarkly::data_model::SDKDataSet data_set) {
    // Optional outside the HasListeners() scope, this allows for the changes
    // to be calculated before the update and then the notification to be
    // sent after the update completes.
    std::optional<DependencySet> change_notifications;
    if (HasListeners()) {
        DependencySet updated_items;

        CalculateChanges(DataKind::kFlag, source_.AllFlags(), data_set.flags,
                         updated_items);
        CalculateChanges(DataKind::kSegment, source_.AllSegments(),
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
    sink_.Init(std::move(data_set));
    // After updating the sink, let listeners know of changes.
    if (change_notifications) {
        NotifyChanges(std::move(*change_notifications));
    }
}

void DataStoreUpdater::Upsert(std::string const& key,
                              data_sources::FlagDescriptor flag) {
    UpsertCommon(DataKind::kFlag, key, source_.GetFlag(key), std::move(flag));
}

void DataStoreUpdater::Upsert(std::string const& key,
                              data_sources::SegmentDescriptor segment) {
    UpsertCommon(DataKind::kSegment, key, source_.GetSegment(key),
                 std::move(segment));
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

DataStoreUpdater::DataStoreUpdater(IDataDestination& sink,
                                   data_sources::IDataSource const& source)
    : sink_(sink), source_(source) {}

}  // namespace launchdarkly::server_side::data_retrieval
