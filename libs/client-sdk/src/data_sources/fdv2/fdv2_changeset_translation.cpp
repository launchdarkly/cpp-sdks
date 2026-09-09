#include "fdv2_changeset_translation.hpp"

#include <launchdarkly/serialization/json_evaluation_result.hpp>

#include <utility>

namespace launchdarkly::client_side::data_sources {

using data_model::ChangeSetType;
using data_model::FDv2Change;
using data_model::FDv2ChangeSet;

// FDv2 "kind" tag for a client-side evaluated flag.
static char const* const kFlagEval = "flag-eval";

std::optional<FlagChangeSet> TranslateChangeSet(FDv2ChangeSet const& change_set,
                                                Logger const& logger) {
    if (change_set.type == ChangeSetType::kNone) {
        return FlagChangeSet{change_set.type, {}, change_set.selector};
    }

    FlagChangeSetData changes;
    changes.reserve(change_set.changes.size());

    for (auto const& change : change_set.changes) {
        if (change.change_type == FDv2Change::ChangeType::kDelete) {
            if (change.kind != kFlagEval) {
                LD_LOG(logger, LogLevel::kWarn)
                    << "FDv2: unknown kind '" << change.kind
                    << "' in delete-object, skipping";
                continue;
            }
            changes.push_back(FlagChange{
                change.key,
                ItemDescriptor{data_model::Tombstone{change.version}}});
            continue;
        }

        if (change.kind != kFlagEval) {
            LD_LOG(logger, LogLevel::kWarn)
                << "FDv2: unknown kind '" << change.kind
                << "' in put-object, skipping";
            continue;
        }

        auto result = ParseEvaluationResult(change.object, change.version);
        if (!result) {
            LD_LOG(logger, LogLevel::kError)
                << "FDv2: could not deserialize flag '" << change.key << "'";
            return std::nullopt;
        }
        if (!result->has_value()) {
            LD_LOG(logger, LogLevel::kWarn) << "FDv2: flag '" << change.key
                                            << "' object was null, skipping";
            continue;
        }
        changes.push_back(
            FlagChange{change.key, ItemDescriptor{std::move(**result)}});
    }

    return FlagChangeSet{change_set.type, std::move(changes),
                         change_set.selector};
}

}  // namespace launchdarkly::client_side::data_sources
