#pragma once

#include "../data_source/data_source_update_sink.hpp"
#include "data_store.hpp"

#include <launchdarkly/server_side/change_notifier.hpp>

#include <boost/signals2/signal.hpp>

#include <memory>

namespace launchdarkly::server_side::data_store {


class DataStoreUpdater
    : public launchdarkly::server_side::data_source::IDataSourceUpdateSink,
      public launchdarkly::server_side::IChangeNotifier {
   public:
    DataStoreUpdater(std::shared_ptr<IDataSourceUpdateSink> sink, std::shared_ptr<IDataStore> store);

    std::unique_ptr<IConnection> OnFlagChange(ChangeHandler handler) override;

    void Init(launchdarkly::data_model::SDKDataSet dataSet) override;
    void Upsert(std::string key, FlagDescriptor flag) override;
    void Upsert(std::string key, SegmentDescriptor segment) override;
    ~DataStoreUpdater() override = default;

   private:
    bool HasListeners() const;

    std::shared_ptr<IDataSourceUpdateSink> sink_;
    std::shared_ptr<IDataStore> store_;

    std::unordered_map<
        std::string,
        boost::signals2::signal<void(std::shared_ptr<ChangeSet>)>>
        signals_;

    // Recursive mutex so that has_listeners can non-conditionally lock
    // the mutex. Otherwise, a pre-condition for the call would be holding
    // the mutex, which is more difficult to keep consistent over the code
    // lifetime.
    mutable std::recursive_mutex signal_mutex_;

};
}  // namespace launchdarkly::server_side::data_store
