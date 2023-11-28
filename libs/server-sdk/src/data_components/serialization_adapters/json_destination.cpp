#include "json_destination.hpp"

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

namespace launchdarkly::server_side::data_components {

using data_interfaces::ISerializedDestination;
using integrations::SerializedItemDescriptor;

FlagKind const JsonDestination::Kinds::Flag = FlagKind();
SegmentKind const JsonDestination::Kinds::Segment = SegmentKind();

/**
 * @brief Creates a boost::json::value representing a tombstone for a given
 * ItemDescriptor. The tombstone includes 'deleted', 'key', and 'version'
 * fields.
 *
 * @param T Type of descriptor.
 * @param key Key of item.
 * @param desc The descriptor.
 * @return Tombstone suitable for serialization.
 */
template <typename T>
boost::json::value Tombstone(std::string const& key,
                             data_model::ItemDescriptor<T> const& desc) {
    boost::json::object tombstone;
    tombstone.emplace("key", key);
    tombstone.emplace("version", desc.version);
    tombstone.emplace("deleted", true);
    return tombstone;
}

/**
 * @brief Creates a SerializedItemDescriptor for a given ItemDescriptor. The
 * SerializedItemDescriptor either represents an item that is present, or one
 * that is absent.
 *
 * @param T Type of descriptor.
 * @param key Key of item.
 * @param desc The descriptor.
 * @return SerializedItemDescriptor suitable for forwarding to an
 * ISerializedDestination.
 */
template <typename T>
SerializedItemDescriptor Serialize(std::string const& key,
                                   data_model::ItemDescriptor<T> const& desc) {
    return desc.item
               ? SerializedItemDescriptor::Present(
                     desc.version, boost::json::serialize(
                                       boost::json::value_from(*desc.item)))
               : SerializedItemDescriptor::Absent(
                     desc.version,
                     boost::json::serialize(Tombstone(key, desc)));
}

JsonDestination::JsonDestination(Logger const& logger,
                                 ISerializedDestination& destination)
    : logger_(logger), dest_(destination) {}

std::string const& JsonDestination::Identity() const {
    static std::string const identity = "(JSON)";
    return identity;
}

void JsonDestination::Init(data_model::SDKDataSet data_set) {
    // TODO(sc-225327): Topographical sort of flag dependencies

    std::vector<ISerializedDestination::ItemCollection> items;

    ISerializedDestination::OrderedNamepace flags;
    for (auto const& [key, descriptor] : data_set.flags) {
        flags.emplace_back(key, Serialize(key, descriptor));
    }
    std::sort(flags.begin(), flags.end(), [](auto const& lhs, auto const& rhs) {
        return lhs.first < rhs.first;
    });
    items.emplace_back(Kinds::Flag, std::move(flags));

    ISerializedDestination::OrderedNamepace segments;
    for (auto const& [key, descriptor] : data_set.segments) {
        segments.emplace_back(key, Serialize(key, descriptor));
    }
    std::sort(
        segments.begin(), segments.end(),
        [](auto const& lhs, auto const& rhs) { return lhs.first < rhs.first; });
    items.emplace_back(Kinds::Segment, std::move(segments));

    if (auto const result = dest_.Init(std::move(items));
        result != ISerializedDestination::InitResult::kSuccess) {
        LD_LOG(logger_, LogLevel::kError)
            << dest_.Identity() << ": failed to store initial SDK data";
    }
}

void JsonDestination::Upsert(std::string const& key,
                             data_model::FlagDescriptor const flag) {
    LogUpsertResult(key, "flag",
                    dest_.Upsert(Kinds::Flag, key, Serialize(key, flag)));
}

void JsonDestination::Upsert(std::string const& key,
                             data_model::SegmentDescriptor const segment) {
    LogUpsertResult(key, "segment",
                    dest_.Upsert(Kinds::Segment, key, Serialize(key, segment)));
}

void JsonDestination::LogUpsertResult(
    std::string const& key,
    std::string const& data_type,
    ISerializedDestination::UpsertResult const& result) const {
    switch (result) {
        case ISerializedDestination::UpsertResult::kSuccess:
            break;
        case ISerializedDestination::UpsertResult::kError:
            LD_LOG(logger_, LogLevel::kError)
                << dest_.Identity() << ": failed to update " << data_type << " "
                << key;
            break;
        case ISerializedDestination::UpsertResult::kNotUpdated:
            LD_LOG(logger_, LogLevel::kDebug)
                << dest_.Identity() << ": " << data_type << " " << key
                << " not updated; data was stale";
            break;
    }
}

}  // namespace launchdarkly::server_side::data_components
