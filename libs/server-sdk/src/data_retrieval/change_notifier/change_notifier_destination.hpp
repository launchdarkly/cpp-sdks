#pragma once

#include "../data_sources/data_destination_interface.hpp"
#include "../data_sources/data_source_interface.hpp"
#include "dependency_tracker.hpp"

#include <launchdarkly/server_side/change_notifier.hpp>

#include <boost/signals2/signal.hpp>

#include <memory>

namespace launchdarkly::server_side::data_system {

class DataStoreUpdater : public IDataDestination, public IChangeNotifier {
   public:
    template <typename Storage>
    using Collection = data_model::SDKDataSet::Collection<std::string, Storage>;

    template <typename Storage>
    using SharedItem = std::shared_ptr<data_model::ItemDescriptor<Storage>>;

    template <typename Storage>
    using SharedCollection =
        std::unordered_map<std::string, SharedItem<Storage>>;

    DataStoreUpdater(IDataDestination& sink,
                     data_sources::IDataSource const& source);

    std::unique_ptr<IConnection> OnFlagChange(ChangeHandler handler) override;

    void Init(launchdarkly::data_model::SDKDataSet data_set) override;
    void Upsert(std::string const& key,
                data_sources::FlagDescriptor flag) override;
    void Upsert(std::string const& key,
                data_sources::SegmentDescriptor segment) override;
    ~DataStoreUpdater() override = default;

    DataStoreUpdater(DataStoreUpdater const& item) = delete;
    DataStoreUpdater(DataStoreUpdater&& item) = delete;
    DataStoreUpdater& operator=(DataStoreUpdater const&) = delete;
    DataStoreUpdater& operator=(DataStoreUpdater&&) = delete;

   private:
    bool HasListeners() const;

    template <typename FlagOrSegment>
    void UpsertCommon(
        DataKind kind,
        std::string key,
        SharedItem<FlagOrSegment> existing,
        launchdarkly::data_model::ItemDescriptor<FlagOrSegment> updated) {
        if (existing && (updated.version <= existing->version)) {
            // Out of order update, ignore it.
            return;
        }

        dependency_tracker_.UpdateDependencies(key, updated);

        if (HasListeners()) {
            auto updated_deps = DependencySet();
            dependency_tracker_.CalculateChanges(kind, key, updated_deps);
            NotifyChanges(updated_deps);
        }

        sink_.Upsert(key, updated);
    }

    template <typename FlagOrSegment>
    void CalculateChanges(
        DataKind kind,
        SharedCollection<FlagOrSegment> const& existing_flags_or_segments,
        Collection<FlagOrSegment> const& new_flags_or_segments,
        DependencySet& updated_items) {
        for (auto const& old_flag_or_segment : existing_flags_or_segments) {
            auto new_flag_or_segment =
                new_flags_or_segments.find(old_flag_or_segment.first);
            if (new_flag_or_segment != new_flags_or_segments.end() &&
                new_flag_or_segment->second.version <=
                    old_flag_or_segment.second->version) {
                continue;
            }

            // Deleted.
            dependency_tracker_.CalculateChanges(
                kind, old_flag_or_segment.first, updated_items);
        }

        for (auto const& flag_or_segment : new_flags_or_segments) {
            auto oldItem =
                existing_flags_or_segments.find(flag_or_segment.first);
            if (oldItem != existing_flags_or_segments.end() &&
                flag_or_segment.second.version <= oldItem->second->version) {
                continue;
            }

            // Updated or new.
            dependency_tracker_.CalculateChanges(kind, flag_or_segment.first,
                                                 updated_items);
        }
    }

    void NotifyChanges(DependencySet changes);

    IDataDestination& sink_;
    data_sources::IDataSource const& source_;

    boost::signals2::signal<void(std::shared_ptr<ChangeSet>)> signals_;

    // Recursive mutex so that has_listeners can non-conditionally lock
    // the mutex. Otherwise, a pre-condition for the call would be holding
    // the mutex, which is more difficult to keep consistent over the code
    // lifetime.
    //
    // Signals themselves are thread-safe, and this mutex only allows us to
    // prevent the addition of listeners between the listener check, calculation
    // and dispatch of events.
    mutable std::recursive_mutex signal_mutex_;

    DependencyTracker dependency_tracker_;
};
}  // namespace launchdarkly::server_side::data_system
