#pragma once

#include "../data_source/data_source_update_sink.hpp"
#include "data_store.hpp"
#include "dependency_tracker.hpp"

#include <launchdarkly/server_side/change_notifier.hpp>

#include <boost/signals2/signal.hpp>

#include <memory>

namespace launchdarkly::server_side::data_store {

class DataStoreUpdater
    : public launchdarkly::server_side::data_source::IDataSourceUpdateSink,
      public launchdarkly::server_side::IChangeNotifier {
   public:
    DataStoreUpdater(std::shared_ptr<IDataSourceUpdateSink> sink,
                     std::shared_ptr<IDataStore> store);

    std::unique_ptr<IConnection> OnFlagChange(ChangeHandler handler) override;

    void Init(launchdarkly::data_model::SDKDataSet data_set) override;
    void Upsert(std::string key, FlagDescriptor flag) override;
    void Upsert(std::string key, SegmentDescriptor segment) override;
    ~DataStoreUpdater() override = default;

    DataStoreUpdater(DataStoreUpdater const& item) = delete;
    DataStoreUpdater(DataStoreUpdater&& item) = delete;
    DataStoreUpdater& operator=(DataStoreUpdater const&) = delete;
    DataStoreUpdater& operator=(DataStoreUpdater&&) = delete;

   private:
    bool HasListeners() const;

    template <typename FlagOrSegmentDescriptor>
    void UpsertCommon(DataKind kind,
                      std::string key,
                      std::shared_ptr<FlagOrSegmentDescriptor> existing,
                      FlagOrSegmentDescriptor updated) {
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

        sink_->Upsert(key, updated);
    }

    template <typename FlagOrSegmentDescriptor>
    void CalculateChanges(
        DataKind kind,
        std::unordered_map<std::string,
                           std::shared_ptr<FlagOrSegmentDescriptor>>
            existing_flags_or_segments,
        std::unordered_map<std::string, FlagOrSegmentDescriptor>
            new_flags_or_segments,
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

    std::shared_ptr<IDataSourceUpdateSink> sink_;
    std::shared_ptr<IDataStore> store_;

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
}  // namespace launchdarkly::server_side::data_store
