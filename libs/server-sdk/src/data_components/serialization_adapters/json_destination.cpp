#include "json_destination.hpp"

namespace launchdarkly::server_side::data_components {

JsonDestination::JsonDestination(
    data_interfaces::ISerializedDestination& destination)
    : dest_(destination) {}

void JsonDestination::Init(data_model::SDKDataSet data_set) {
    // TODO: serialize and forward to dest_.Init
}

void JsonDestination::Upsert(std::string const& key,
                             data_model::FlagDescriptor flag) {
    // TODO: serialize and forward to dest_.Upsert
}

void JsonDestination::Upsert(std::string const& key,
                             data_model::SegmentDescriptor segment) {
    // TODO: serialize and forward to dest_.Upsert
}

std::string JsonDestination::Identity() const {
    return dest_.Identity();
}
}  // namespace launchdarkly::server_side::data_components
