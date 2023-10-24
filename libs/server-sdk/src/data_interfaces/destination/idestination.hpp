#pragma once

#include <launchdarkly/data_model/descriptors.hpp>
#include <launchdarkly/data_model/sdk_data_set.hpp>

namespace launchdarkly::server_side::data_interfaces {

class IDestination {
   public:
    virtual void Init(data_model::SDKDataSet data_set) = 0;
    virtual void Upsert(std::string const& key,
                        data_model::FlagDescriptor flag) = 0;
    virtual void Upsert(std::string const& key,
                        data_model::SegmentDescriptor segment) = 0;
    virtual std::string const& Identity() const = 0;

    IDestination(IDestination const& item) = delete;
    IDestination(IDestination&& item) = delete;
    IDestination& operator=(IDestination const&) = delete;
    IDestination& operator=(IDestination&&) = delete;
    virtual ~IDestination() = default;

   protected:
    IDestination() = default;
};
}  // namespace launchdarkly::server_side::data_interfaces
