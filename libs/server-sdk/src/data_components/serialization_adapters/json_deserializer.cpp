#include "json_deserializer.hpp"

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

#include <launchdarkly/server_side/integrations/data_reader/serialized_item_descriptor.hpp>

#include <boost/json.hpp>

namespace launchdarkly::server_side::data_components {

JsonDeserializer::JsonDeserializer(
    Logger const& logger,
    std::shared_ptr<integrations::ISerializedDataReader> reader)
    : logger_(logger),
      flag_kind_(),
      segment_kind_(),
      source_(std::move(reader)),
      identity_(source_->Identity() + " (JSON)") {}

data_interfaces::IDataReader::Single<data_model::FlagDescriptor>
JsonDeserializer::GetFlag(std::string const& key) const {
    return DeserializeSingle<data_model::Flag>(flag_kind_, key);
}

data_interfaces::IDataReader::Single<data_model::SegmentDescriptor>
JsonDeserializer::GetSegment(std::string const& key) const {
    return DeserializeSingle<data_model::Segment>(segment_kind_, key);
}

data_interfaces::IDataReader::Collection<data_model::FlagDescriptor>
JsonDeserializer::AllFlags() const {
    return DeserializeCollection<data_model::Flag>(flag_kind_);
}

data_interfaces::IDataReader::Collection<data_model::SegmentDescriptor>
JsonDeserializer::AllSegments() const {
    return DeserializeCollection<data_model::Segment>(segment_kind_);
}

std::string const& JsonDeserializer::Identity() const {
    return identity_;
}

bool JsonDeserializer::Initialized() const {
    return source_->Initialized();
}

}  // namespace launchdarkly::server_side::data_components
