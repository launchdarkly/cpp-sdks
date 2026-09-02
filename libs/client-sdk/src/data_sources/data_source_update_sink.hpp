#pragma once

#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <launchdarkly/client_side/data_source_status.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_result.hpp>
#include <launchdarkly/data_model/change_set.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>

namespace launchdarkly::client_side {

using ItemDescriptor = data_model::ItemDescriptor<EvaluationResult>;

/**
 * The new state of one flag within a changeset: either an evaluation result
 * or a tombstone marking the flag deleted.
 */
struct FlagChange {
    std::string key;
    ItemDescriptor item;
};

using FlagChangeSetData = std::vector<FlagChange>;

/**
 * A set of flag changes to apply as a unit, along with the selector
 * identifying the resulting data set. A full changeset replaces the flag
 * data, a partial changeset merges into it, and a "none" changeset confirms
 * the existing data is current.
 */
using FlagChangeSet = data_model::ChangeSet<FlagChangeSetData>;

/**
 * Interface for handling updates from LaunchDarkly.
 */
class IDataSourceUpdateSink {
   public:
    virtual void Init(Context const& context,
                      std::unordered_map<std::string, ItemDescriptor> data) = 0;
    virtual void Upsert(Context const& context,
                        std::string key,
                        ItemDescriptor item) = 0;

    IDataSourceUpdateSink(IDataSourceUpdateSink const& item) = delete;
    IDataSourceUpdateSink(IDataSourceUpdateSink&& item) = delete;
    IDataSourceUpdateSink& operator=(IDataSourceUpdateSink const&) = delete;
    IDataSourceUpdateSink& operator=(IDataSourceUpdateSink&&) = delete;
    virtual ~IDataSourceUpdateSink() = default;

   protected:
    IDataSourceUpdateSink() = default;
};

}  // namespace launchdarkly::client_side
