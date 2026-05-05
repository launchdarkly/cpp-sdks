#include "change_notifier.hpp"

#include <launchdarkly/signals/boost_signal_connection.hpp>

#include <mutex>
#include <utility>
#include <variant>

namespace launchdarkly::server_side::data_components {

namespace {

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace

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

void ChangeNotifier::Apply(
    data_model::ChangeSet<data_interfaces::ChangeSetData> change_set) {
    if (change_set.type == data_model::ChangeSetType::kNone) {
        return;
    }

    // Compute changed dependencies before passing the changeset to the sink.
    std::optional<DependencySet> change_notifications;
    if (HasListeners()) {
        DependencySet affected;
        if (change_set.type == data_model::ChangeSetType::kFull) {
            // Group items by kind so the existing per-kind diff helper can
            // compare the new state to the existing store contents.
            Collection<data_model::Flag> new_flags;
            Collection<data_model::Segment> new_segments;
            for (auto const& change : change_set.data) {
                std::visit(
                    overloaded{
                        [&](data_model::ItemDescriptor<data_model::Flag> const&
                                f) { new_flags.emplace(change.key, f); },
                        [&](data_model::ItemDescriptor<
                            data_model::Segment> const& s) {
                            new_segments.emplace(change.key, s);
                        },
                    },
                    change.object);
            }
            CalculateChanges(DataKind::kFlag, source_.AllFlags(), new_flags,
                             affected);
            CalculateChanges(DataKind::kSegment, source_.AllSegments(),
                             new_segments, affected);
        } else {
            // Partial: every item in the changeset is treated as a change;
            // no version comparison.
            for (auto const& change : change_set.data) {
                std::visit(overloaded{
                               [&](data_model::ItemDescriptor<
                                   data_model::Flag> const&) {
                                   dependency_tracker_.CalculateChanges(
                                       DataKind::kFlag, change.key, affected);
                               },
                               [&](data_model::ItemDescriptor<
                                   data_model::Segment> const&) {
                                   dependency_tracker_.CalculateChanges(
                                       DataKind::kSegment, change.key,
                                       affected);
                               },
                           },
                           change.object);
            }
        }
        change_notifications = std::move(affected);
    }

    // Update the dependency tracker.
    if (change_set.type == data_model::ChangeSetType::kFull) {
        dependency_tracker_.Clear();
    }
    for (auto const& change : change_set.data) {
        std::visit(
            [&](auto const& descriptor) {
                dependency_tracker_.UpdateDependencies(change.key, descriptor);
            },
            change.object);
    }

    sink_.Apply(std::move(change_set));

    if (change_notifications) {
        NotifyChanges(std::move(*change_notifications));
    }
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

ChangeNotifier::ChangeNotifier(data_interfaces::ITransactionalDestination& sink,
                               data_interfaces::IStore const& source)
    : sink_(sink), source_(source) {}

std::string const& ChangeNotifier::Identity() const {
    static std::string const identity =
        "change notifier for " + sink_.Identity();
    return identity;
}

}  // namespace launchdarkly::server_side::data_components
