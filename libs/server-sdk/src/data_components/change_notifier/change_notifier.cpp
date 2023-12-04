#include "change_notifier.hpp"

#include <launchdarkly/signals/boost_signal_connection.hpp>
#include <mutex>

namespace launchdarkly::server_side::data_components {

std::unique_ptr<IConnection> ChangeNotifier::OnFlagChange(
    ChangeHandler handler) {
    std::lock_guard lock{signal_mutex_};

    return std::make_unique<launchdarkly::internal::signals::SignalConnection>(
        signals_.connect(handler));
}

void ChangeNotifier::Init(data_model::SDKDataSet data_set) {
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

void ChangeNotifier::Upsert(std::string const& key,
                            data_model::FlagDescriptor flag) {
    UpsertCommon(DataKind::kFlag, key, source_.GetFlag(key), std::move(flag));
}

void ChangeNotifier::Upsert(std::string const& key,
                            data_model::SegmentDescriptor segment) {
    UpsertCommon(DataKind::kSegment, key, source_.GetSegment(key),
                 std::move(segment));
}

bool ChangeNotifier::HasListeners() const {
    std::lock_guard lock{signal_mutex_};
    return !signals_.empty();
}

void ChangeNotifier::NotifyChanges(DependencySet changes) {
    std::lock_guard lock{signal_mutex_};
    auto flag_changes = changes.SetForKind(DataKind::kFlag);
    // Only emit an event if there are changes.
    if (!flag_changes.empty()) {
        signals_(std::make_shared<ChangeSet>(std::move(flag_changes)));
    }
}

ChangeNotifier::ChangeNotifier(IDestination& sink,
                               data_interfaces::IStore const& source)
    : sink_(sink), source_(source) {}

std::string const& ChangeNotifier::Identity() const {
    static std::string const identity =
        "change notifier for " + sink_.Identity();
    return identity;
}

}  // namespace launchdarkly::server_side::data_components
