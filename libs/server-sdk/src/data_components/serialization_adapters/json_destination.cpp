#include "json_destination.hpp"

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

namespace launchdarkly::server_side::data_components {

using integrations::SerializedItemDescriptor;

FlagKind const JsonDestination::Kinds::Flag = FlagKind();
SegmentKind const JsonDestination::Kinds::Segment = SegmentKind();

JsonDestination::JsonDestination(
    data_interfaces::ISerializedDestination& destination)
    : dest_(destination) {}

void JsonDestination::Init(data_model::SDKDataSet data_set) {
    // TODO: implement
}

void JsonDestination::Upsert(std::string const& key,
                             data_model::FlagDescriptor flag) {
    SerializedItemDescriptor descriptor;

    if (!flag.item) {
        boost::json::object tombstone;
        tombstone.emplace("deleted", true);
        tombstone.emplace("key", key);
        tombstone.emplace("version", flag.version);

        descriptor = SerializedItemDescriptor::Absent(
            flag.version, boost::json::serialize(tombstone));
    } else {
        descriptor = SerializedItemDescriptor::Present(
            flag.version,
            boost::json::serialize(boost::json::value_from(*flag.item)));
    }

    // TOOD: Log upsert errors?

    auto _ = dest_.Upsert(Kinds::Flag, key, std::move(descriptor));
}

void JsonDestination::Upsert(std::string const& key,
                             data_model::SegmentDescriptor segment) {
    SerializedItemDescriptor descriptor;

    if (!segment.item) {
        boost::json::object tombstone;
        tombstone.emplace("deleted", true);
        tombstone.emplace("key", key);
        tombstone.emplace("version", segment.version);

        descriptor = SerializedItemDescriptor::Absent(
            segment.version, boost::json::serialize(tombstone));
    } else {
        descriptor = SerializedItemDescriptor::Present(
            segment.version,
            boost::json::serialize(boost::json::value_from(*segment.item)));
    }
    // TOOD: Log upsert errors?

    auto _ = dest_.Upsert(Kinds::Segment, key, std::move(descriptor));
}

std::string const& JsonDestination::Identity() const {
    return dest_.Identity();
}
}  // namespace launchdarkly::server_side::data_components
