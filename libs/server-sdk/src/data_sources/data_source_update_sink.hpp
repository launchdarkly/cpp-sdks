#pragma once

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/sdk_data_set.hpp>
#include <launchdarkly/data_model/segment.hpp>

namespace launchdarkly::server_side::data_source {
/**
 * Interface for handling updates from LaunchDarkly.
 */
class IDataSourceUpdateSink {
   public:
    using FlagDescriptor = launchdarkly::data_model::ItemDescriptor<
        launchdarkly::data_model::Flag>;
    using SegmentDescriptor = launchdarkly::data_model::ItemDescriptor<
        launchdarkly::data_model::Segment>;

    virtual void Init(launchdarkly::data_model::SDKDataSet data_set) = 0;
    virtual void Upsert(std::string key, FlagDescriptor flag) = 0;
    virtual void Upsert(std::string key, SegmentDescriptor segment) = 0;

    IDataSourceUpdateSink(IDataSourceUpdateSink const& item) = delete;
    IDataSourceUpdateSink(IDataSourceUpdateSink&& item) = delete;
    IDataSourceUpdateSink& operator=(IDataSourceUpdateSink const&) = delete;
    IDataSourceUpdateSink& operator=(IDataSourceUpdateSink&&) = delete;
    virtual ~IDataSourceUpdateSink() = default;

   protected:
    IDataSourceUpdateSink() = default;
};
}  // namespace launchdarkly::server_side::data_source
