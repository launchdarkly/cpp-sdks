#pragma once

#include "../data_source_update_sink.hpp"

#include <launchdarkly/data_model/fdv2_change.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <optional>

namespace launchdarkly::client_side::data_sources {

/**
 * Translates an FDv2ChangeSet into flag changes ready to apply to the store.
 *
 * Unknown kinds are logged and skipped, for forward compatibility. Returns
 * nullopt if a flag-eval object fails to deserialize.
 */
std::optional<FlagChangeSet> TranslateChangeSet(
    data_model::FDv2ChangeSet const& change_set,
    Logger const& logger);

}  // namespace launchdarkly::client_side::data_sources
