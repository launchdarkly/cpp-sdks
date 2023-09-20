#pragma once

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/sdk_data_set.hpp>
#include <launchdarkly/data_model/segment.hpp>

#include "descriptors.hpp"

namespace launchdarkly::server_side::data_sources {
/**
 * Interface for handling updates from LaunchDarkly.
 */
class IDataSourceUpdateSink {
   public:
    virtual void Init(launchdarkly::data_model::SDKDataSet data_set) = 0;
    virtual void Upsert(std::string const& key, FlagDescriptor flag) = 0;
    virtual void Upsert(std::string const& key, SegmentDescriptor segment) = 0;

    IDataSourceUpdateSink(IDataSourceUpdateSink const& item) = delete;
    IDataSourceUpdateSink(IDataSourceUpdateSink&& item) = delete;
    IDataSourceUpdateSink& operator=(IDataSourceUpdateSink const&) = delete;
    IDataSourceUpdateSink& operator=(IDataSourceUpdateSink&&) = delete;
    virtual ~IDataSourceUpdateSink() = default;

   protected:
    IDataSourceUpdateSink() = default;
};
}  // namespace launchdarkly::server_side::data_sources
