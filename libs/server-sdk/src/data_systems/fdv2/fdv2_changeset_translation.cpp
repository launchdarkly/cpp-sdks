#include "fdv2_changeset_translation.hpp"

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/segment.hpp>
#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

#include <boost/json.hpp>
#include <tl/expected.hpp>

namespace launchdarkly::server_side::data_systems {

using data_interfaces::ChangeSetData;
using data_interfaces::ItemChange;
using data_model::ChangeSet;
using data_model::ChangeSetType;
using data_model::FDv2ChangeSet;

static std::optional<ItemChange> TranslateDelete(
    data_model::FDv2Change const& change,
    Logger const& logger) {
    if (change.kind == "flag") {
        return ItemChange{change.key,
                          data_model::ItemDescriptor<data_model::Flag>{
                              data_model::Tombstone{change.version}}};
    }
    if (change.kind == "segment") {
        return ItemChange{change.key,
                          data_model::ItemDescriptor<data_model::Segment>{
                              data_model::Tombstone{change.version}}};
    }
    LD_LOG(logger, LogLevel::kWarn) << "FDv2: unknown kind '" << change.kind
                                    << "' in delete-object, skipping";
    return std::nullopt;
}

static bool TranslatePutFlag(data_model::FDv2Change const& change,
                             ChangeSetData* changes,
                             Logger const& logger) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<data_model::Flag>, JsonError>>(
        change.object);
    if (!result) {
        LD_LOG(logger, LogLevel::kError)
            << "FDv2: could not deserialize flag '" << change.key << "'";
        return false;
    }
    if (!result->has_value()) {
        LD_LOG(logger, LogLevel::kWarn)
            << "FDv2: flag '" << change.key << "' object was null, skipping";
        return true;
    }
    changes->push_back(ItemChange{
        change.key,
        data_model::ItemDescriptor<data_model::Flag>{std::move(**result)}});
    return true;
}

static bool TranslatePutSegment(data_model::FDv2Change const& change,
                                ChangeSetData* changes,
                                Logger const& logger) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<data_model::Segment>, JsonError>>(
        change.object);
    if (!result) {
        LD_LOG(logger, LogLevel::kError)
            << "FDv2: could not deserialize segment '" << change.key << "'";
        return false;
    }
    if (!result->has_value()) {
        LD_LOG(logger, LogLevel::kWarn)
            << "FDv2: segment '" << change.key << "' object was null, skipping";
        return true;
    }
    changes->push_back(ItemChange{
        change.key,
        data_model::ItemDescriptor<data_model::Segment>{std::move(**result)}});
    return true;
}

std::optional<ChangeSet<ChangeSetData>> TranslateChangeSet(
    FDv2ChangeSet const& change_set,
    Logger const& logger) {
    if (change_set.type == ChangeSetType::kNone) {
        return ChangeSet<ChangeSetData>{
            change_set.type, {}, change_set.selector};
    }

    ChangeSetData changes;
    changes.reserve(change_set.changes.size());

    for (auto const& change : change_set.changes) {
        if (change.change_type == data_model::FDv2Change::ChangeType::kDelete) {
            if (auto item = TranslateDelete(change, logger)) {
                changes.push_back(std::move(*item));
            }
        } else {
            if (change.kind == "flag") {
                if (!TranslatePutFlag(change, &changes, logger)) {
                    return std::nullopt;
                }
            } else if (change.kind == "segment") {
                if (!TranslatePutSegment(change, &changes, logger)) {
                    return std::nullopt;
                }
            } else {
                LD_LOG(logger, LogLevel::kWarn)
                    << "FDv2: unknown kind '" << change.kind
                    << "' in put-object, skipping";
            }
        }
    }

    return ChangeSet<ChangeSetData>{change_set.type, std::move(changes),
                                    change_set.selector};
}

}  // namespace launchdarkly::server_side::data_systems
