#pragma once

#include <launchdarkly/data_model/descriptors.hpp>
#include <launchdarkly/data_model/sdk_data_set.hpp>

namespace launchdarkly::server_side::data_sources {
/**
 * Interface for handling updates from LaunchDarkly.
 */
class IDataSourceUpdateSink {
   public:
    virtual void Init(data_model::SDKDataSet data_set) = 0;
    virtual void Upsert(std::string const& key,
                        data_model::FlagDescriptor flag) = 0;
    virtual void Upsert(std::string const& key,
                        data_model::SegmentDescriptor segment) = 0;

    IDataSourceUpdateSink(IDataSourceUpdateSink const& item) = delete;
    IDataSourceUpdateSink(IDataSourceUpdateSink&& item) = delete;
    IDataSourceUpdateSink& operator=(IDataSourceUpdateSink const&) = delete;
    IDataSourceUpdateSink& operator=(IDataSourceUpdateSink&&) = delete;
    virtual ~IDataSourceUpdateSink() = default;

   protected:
    IDataSourceUpdateSink() = default;
};
}  // namespace launchdarkly::server_side::data_sources
