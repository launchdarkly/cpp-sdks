#include "json_deserializer.hpp"

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

#include <launchdarkly/server_side/integrations/serialized_item_descriptor.hpp>

#include <boost/json.hpp>

namespace launchdarkly::server_side::data_components {

JsonDeserializer::JsonDeserializer(
    std::shared_ptr<data_interfaces::ISerializedDataReader> reader)
    : flag_kind_(), segment_kind_(), reader_(std::move(reader)) {}

data_interfaces::IDataReader::Single<data_model::FlagDescriptor>
JsonDeserializer::GetFlag(std::string const& key) const {
    return Deserialize<data_model::Flag>(flag_kind_, key);
}

data_interfaces::IDataReader::Single<data_model::SegmentDescriptor>
JsonDeserializer::GetSegment(std::string const& key) const {
    return Deserialize<data_model::Segment>(segment_kind_, key);
}

data_interfaces::IDataReader::Collection<data_model::FlagDescriptor>
JsonDeserializer::AllFlags() const {
    return DeserializeAll<data_model::Flag>(flag_kind_);
}

data_interfaces::IDataReader::Collection<data_model::SegmentDescriptor>
JsonDeserializer::AllSegments() const {
    return DeserializeAll<data_model::Segment>(segment_kind_);
}

std::string const& JsonDeserializer::Identity() const {
    static std::string const name = reader_->Identity() + " (JSON)";
    return name;
}

bool JsonDeserializer::Initialized() const {
    return reader_->Initialized();
}

}  // namespace launchdarkly::server_side::data_components
