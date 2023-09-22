#pragma once

#include <launchdarkly/data_model/descriptors.hpp>
#include <launchdarkly/data_model/sdk_data_set.hpp>

namespace launchdarkly::server_side::data_retrieval {

class IDataDestination {
   public:
    virtual void Init(data_model::SDKDataSet data_set) = 0;
    virtual void Upsert(std::string const& key,
                        data_model::FlagDescriptor flag) = 0;
    virtual void Upsert(std::string const& key,
                        data_model::SegmentDescriptor segment) = 0;
    virtual std::string const& Identity() const = 0;

    IDataDestination(IDataDestination const& item) = delete;
    IDataDestination(IDataDestination&& item) = delete;
    IDataDestination& operator=(IDataDestination const&) = delete;
    IDataDestination& operator=(IDataDestination&&) = delete;
    virtual ~IDataDestination() = default;

   protected:
    IDataDestination() = default;
};
}  // namespace launchdarkly::server_side::data_retrieval
