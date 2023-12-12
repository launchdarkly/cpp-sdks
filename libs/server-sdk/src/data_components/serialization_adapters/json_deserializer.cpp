#include "json_deserializer.hpp"

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

#include <launchdarkly/server_side/integrations/data_reader/serialized_item_descriptor.hpp>

#include <boost/json.hpp>

namespace launchdarkly {
tl::expected<std::optional<data_model::Tombstone>, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::optional<data_model::Tombstone>,
                                           JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);

    auto const& obj = json_value.as_object();

    bool deleted = false;
    PARSE_REQUIRED_FIELD(deleted, obj, "deleted");

    if (!deleted) {
        return tl::make_unexpected(JsonError::kTombstoneInvalidDeletedField);
    }

    std::uint64_t version = 0;
    PARSE_REQUIRED_FIELD(version, obj, "version");

    return data_model::Tombstone(version);
}
}  // namespace launchdarkly

namespace launchdarkly::server_side::data_components {

JsonDeserializer::JsonDeserializer(
    Logger const& logger,
    std::shared_ptr<integrations::ISerializedDataReader> reader)
    : logger_(logger),
      flag_kind_(),
      segment_kind_(),
      source_(std::move(reader)),
      identity_(source_->Identity() + " (JSON)") {}

data_interfaces::IDataReader::SingleResult<data_model::Flag>
JsonDeserializer::GetFlag(std::string const& key) const {
    return DeserializeSingle<data_model::Flag>(flag_kind_, key);
}

data_interfaces::IDataReader::SingleResult<data_model::Segment>
JsonDeserializer::GetSegment(std::string const& key) const {
    return DeserializeSingle<data_model::Segment>(segment_kind_, key);
}

data_interfaces::IDataReader::CollectionResult<data_model::Flag>
JsonDeserializer::AllFlags() const {
    return DeserializeCollection<data_model::Flag>(flag_kind_);
}

data_interfaces::IDataReader::CollectionResult<data_model::Segment>
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
