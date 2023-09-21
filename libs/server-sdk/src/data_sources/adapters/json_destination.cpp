#include "json_destination.hpp"

namespace launchdarkly::server_side::data_sources::adapters {

JsonDestination::JsonDestination(ISerializedDataDestination& destination)
    : dest_(destination) {}

void JsonDestination::Init(data_model::SDKDataSet data_set) {
    // TODO: serialize and forward to dest_.Init
}

void JsonDestination::Upsert(std::string const& key, FlagDescriptor flag) {
    // TODO: serialize and forward to dest_.Upsert
}

void JsonDestination::Upsert(std::string const& key,
                             SegmentDescriptor segment) {
    // TODO: serialize and forward to dest_.Upsert
}

std::string JsonDestination::Identity() const {
    return dest_.Identity();
}
}  // namespace launchdarkly::server_side::data_sources::adapters
