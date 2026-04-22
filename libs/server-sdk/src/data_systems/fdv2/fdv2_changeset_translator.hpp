#pragma once

#include "../../data_interfaces/item_change.hpp"

#include <launchdarkly/data_model/change_set.hpp>
#include <launchdarkly/data_model/fdv2_change.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <optional>

namespace launchdarkly::server_side::data_systems {

class FDv2ChangeSetTranslator {
   public:
    /**
     * Translates a changeset into typed changes ready to apply to the store.
     *
     * Unknown kinds are warned and skipped. If any known kind fails to
     * deserialize, the entire changeset is aborted and nullopt is returned.
     */
    std::optional<data_model::ChangeSet<data_interfaces::ChangeSetData>>
    Translate(data_model::FDv2ChangeSet const& change_set,
              Logger const& logger) const;
};

}  // namespace launchdarkly::server_side::data_systems
