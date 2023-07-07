#pragma once

#include "../data_source/data_source_update_sink.hpp"

#include <launchdarkly/server_side/change_notifier.hpp>

namespace launchdarkly::server_side::data_store {
class DataStoreUpdater
    : public launchdarkly::server_side::data_source::IDataSourceUpdateSink,
      public launchdarkly::server_side::IChangeNotifier {
   public:
    void Init(launchdarkly::data_model::SDKDataSet dataSet) override;
    void Upsert(std::string key, FlagDescriptor flag) override;
    void Upsert(std::string key, SegmentDescriptor segment) override;
};
}  // namespace launchdarkly::server_side::data_store
