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
    tombstone.emplace("deleted", true);
    tombstone.emplace("key", key);
    tombstone.emplace("version", desc.version);
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

JsonDestination::JsonDestination(ISerializedDestination& destination)
    : dest_(destination) {}

void JsonDestination::Init(data_model::SDKDataSet data_set) {
    // TODO: implement

    std::vector<ISerializedDestination::ItemCollection> items;

    ISerializedDestination::OrderedNamepace flags;
    for (auto const& [key, descriptor] : data_set.flags) {
        flags.emplace_back(key, Serialize(key, descriptor));
    }
    std::sort(flags.begin(), flags.end());
    items.emplace_back(Kinds::Flag, std::move(flags));

    ISerializedDestination::OrderedNamepace segments;
    for (auto const& [key, descriptor] : data_set.segments) {
        segments.emplace_back(key, Serialize(key, descriptor));
    }
    std::sort(segments.begin(), segments.end());
    items.emplace_back(Kinds::Segment, std::move(segments));

    dest_.Init(std::move(items));
}

void JsonDestination::Upsert(std::string const& key,
                             data_model::FlagDescriptor flag) {
    // TODO: how to handle errors?
    dest_.Upsert(Kinds::Flag, key, Serialize(key, flag));
}

void JsonDestination::Upsert(std::string const& key,
                             data_model::SegmentDescriptor segment) {
    // TODO: how to handle errors?
    dest_.Upsert(Kinds::Segment, key, Serialize(key, segment));
}

std::string const& JsonDestination::Identity() const {
    static std::string const identity = dest_.Identity() + "(JSON)";
    return identity;
}

}  // namespace launchdarkly::server_side::data_components
