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

    void Init(launchdarkly::data_model::SDKDataSet dataSet) override;
    void Upsert(std::string key, FlagDescriptor flag) override;
    void Upsert(std::string key, SegmentDescriptor segment) override;
    ~DataStoreUpdater() override = default;

   private:
    bool HasListeners() const;

    template <typename FlagOrSegmentDescriptor>
    void UpsertCommon(DataKind kind,
                      std::string key,
                      std::shared_ptr<FlagOrSegmentDescriptor> existing,
                      FlagOrSegmentDescriptor updated) {
        if (existing && (existing->version > updated.version)) {
            // Out of order update, ignore it.
            return;
        }

        dependencyTracker_.UpdateDependencies(key, updated);

        if (HasListeners()) {
            auto updatedDeps = DependencySet();
            dependencyTracker_.CalculateChanges(kind, key, updatedDeps);
            NotifyChanges(updatedDeps);
        }

        sink_->Upsert(key, updated);
    }

    template <typename FlagOrSegmentDescriptor>
    void CalculateChanges(
        DataKind kind,
        std::unordered_map<std::string,
                           std::shared_ptr<FlagOrSegmentDescriptor>>
            existingFlagsOrSegments,
        std::unordered_map<std::string, FlagOrSegmentDescriptor>
            newFlagsOrSegments,
        DependencySet& updatedItems) {
        for (auto const& flagOrSegment : newFlagsOrSegments) {
            auto oldItem = existingFlagsOrSegments.find(flagOrSegment.first);
            if (oldItem != existingFlagsOrSegments.end()) {
                if (flagOrSegment.second.version > oldItem->second->version) {
                    dependencyTracker_.CalculateChanges(
                        kind, flagOrSegment.first, updatedItems);
                }
            }
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

    DependencyTracker dependencyTracker_;
};
}  // namespace launchdarkly::server_side::data_store
