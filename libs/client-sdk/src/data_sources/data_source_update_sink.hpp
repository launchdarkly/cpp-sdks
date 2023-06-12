#pragma once

#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>

#include <launchdarkly/client_side/data_source_status.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_result.hpp>
#include <launchdarkly/data_kinds/item_descriptor.hpp>

namespace launchdarkly::client_side {

using FlagItemDescriptor = ItemDescriptor<EvaluationResult>;

/**
 * Interface for handling updates from LaunchDarkly.
 */
class IDataSourceUpdateSink {
   public:
    virtual void Init(
        Context const& context,
        std::unordered_map<std::string, FlagItemDescriptor> data) = 0;
    virtual void Upsert(Context const& context,
                        std::string key,
                        FlagItemDescriptor item) = 0;

    IDataSourceUpdateSink(IDataSourceUpdateSink const& item) = delete;
    IDataSourceUpdateSink(IDataSourceUpdateSink&& item) = delete;
    IDataSourceUpdateSink& operator=(IDataSourceUpdateSink const&) = delete;
    IDataSourceUpdateSink& operator=(IDataSourceUpdateSink&&) = delete;
    virtual ~IDataSourceUpdateSink() = default;

   protected:
    IDataSourceUpdateSink() = default;
};

}  // namespace launchdarkly::client_side
