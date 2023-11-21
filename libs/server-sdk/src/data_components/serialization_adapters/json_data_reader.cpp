#include "json_data_reader.hpp"

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

#include <launchdarkly/server_side/integrations/serialized_item_descriptor.hpp>

#include <boost/json.hpp>

namespace launchdarkly::server_side::data_components {

JsonDataReader::JsonDataReader(data_interfaces::ISerializedDataReader& reader)
    : flag_kind_(), segment_kind_(), reader_(reader) {}

data_interfaces::IDataReader::Single<data_model::FlagDescriptor>
JsonDataReader::GetFlag(std::string const& key) const {
    return Deserialize<data_model::Flag>(flag_kind_, key);
}

data_interfaces::IDataReader::Single<data_model::SegmentDescriptor>
JsonDataReader::GetSegment(std::string const& key) const {
    return Deserialize<data_model::Segment>(segment_kind_, key);
}

data_interfaces::IDataReader::Collection<data_model::FlagDescriptor>
JsonDataReader::AllFlags() const {
    // TODO: deserialize then return
    return tl::make_unexpected("Not implemented");
}

data_interfaces::IDataReader::Collection<data_model::SegmentDescriptor>
JsonDataReader::AllSegments() const {
    // TODO: deserialize then return
    return tl::make_unexpected("Not implemented");
}

std::string const& JsonDataReader::Identity() const {
    return reader_.Identity();
}

}  // namespace launchdarkly::server_side::data_components
