#include "json_pull_source.hpp"

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

#include <launchdarkly/server_side/integrations/serialized_descriptors.hpp>

#include <boost/json.hpp>

namespace launchdarkly::server_side::data_components {

JsonSource::JsonSource(data_interfaces::ISerializedDataPullSource& json_source)
    : flag_kind_(), segment_kind_(), source_(json_source) {}

template <typename TData>
static std::optional<data_model::ItemDescriptor<TData>> Deserialize(
    integrations::SerializedItemDescriptor item) {
    if (item.deleted) {
        return data_model::ItemDescriptor<TData>(item.version);
    }

    boost::json::error_code error_code;
    if (!item.serializedItem.has_value()) {
        return std::nullopt;
    }
    auto parsed = boost::json::parse(item.serializedItem.value(), error_code);

    if (error_code) {
        return std::nullopt;
    }

    auto res =
        boost::json::value_to<tl::expected<std::optional<TData>, JsonError>>(
            parsed);

    if (res.has_value() && res->has_value()) {
        return data_model::ItemDescriptor(res->value());
    }

    return std::nullopt;
}

data_interfaces::IPullSource::ItemResult<data_model::FlagDescriptor>
JsonSource::GetFlag(std::string const& key) const {
    return Deserialize<data_model::Flag>(flag_kind_, key);
}

data_interfaces::IPullSource::ItemResult<data_model::SegmentDescriptor>
JsonSource::GetSegment(std::string const& key) const {
    return Deserialize<data_model::Segment>(segment_kind_, key);
}

std::unordered_map<std::string, data_model::FlagDescriptor>
JsonSource::AllFlags() const {
    // TODO: deserialize then return
    data_interfaces::ISerializedDataPullSource::AllResult result =
        source_.All(flag_kind_);
}
std::unordered_map<std::string, data_model::SegmentDescriptor>
JsonSource::AllSegments() const {
    // TODO: deserialize then return

    data_interfaces::ISerializedDataPullSource::AllResult result =
        source_.All(segment_kind_);
}

std::string const& JsonSource::Identity() const {
    return source_.Identity();
}

bool JsonSource::Initialized() const {
    return source_.Initialized();
}

}  // namespace launchdarkly::server_side::data_components
