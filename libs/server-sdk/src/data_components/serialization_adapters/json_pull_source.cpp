#include "json_pull_source.hpp"
#include <launchdarkly/server_side/integrations/serialized_descriptors.hpp>

namespace launchdarkly::server_side::data_components {

JsonSource::JsonSource(data_interfaces::ISerializedDataPullSource& json_source)
    : source_(json_source) {}

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

data_model::FlagDescriptor JsonSource::GetFlag(std::string const& key) const {
    // TODO: deserialize then return
    data_interfaces::ISerializedDataPullSource::GetResult result =
        source_.Get(kind, key);
}
data_model::SegmentDescriptor JsonSource::GetSegment(
    std::string const& key) const {
    // TODO: deserialize then return
    data_interfaces::ISerializedDataPullSource::GetResult result =
        source_.Get(kind, key);
}
std::unordered_map<std::string, data_model::FlagDescriptor>
JsonSource::AllFlags() const {
    // TODO: deserialize then return
    data_interfaces::ISerializedDataPullSource::GetResult result =
        source_.All(kind);
}
std::unordered_map<std::string, data_model::SegmentDescriptor>
JsonSource::AllSegments() const {
    // TODO: deserialize then return

    data_interfaces::ISerializedDataPullSource::GetResult result =
        source_.All(kind);
}

std::string const& JsonSource::Identity() const {
    return source_.Identity();
}

}  // namespace launchdarkly::server_side::data_components
