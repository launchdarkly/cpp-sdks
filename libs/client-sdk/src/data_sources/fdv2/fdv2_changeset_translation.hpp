#pragma once

#include "../data_source_update_sink.hpp"

#include <launchdarkly/data_model/fdv2_change.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <optional>

namespace launchdarkly::client_side::data_sources {

/**
 * Translates an FDv2ChangeSet into flag changes ready to apply to the store.
 *
 * Changes for kinds the client does not evaluate are logged and skipped, so
 * that a new object kind does not break an older SDK. If a flag evaluation
 * result fails to deserialize, the entire changeset is abandoned and nullopt
 * is returned. The caller reports a data source error rather than applying a
 * partial payload.
 */
std::optional<FlagChangeSet> TranslateChangeSet(
    data_model::FDv2ChangeSet const& change_set,
    Logger const& logger);

}  // namespace launchdarkly::client_side::data_sources
