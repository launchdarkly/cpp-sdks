#include "json_pull_source.hpp"

namespace launchdarkly::server_side::data::adapters {

template <typename TData>
static std::optional<data_model::ItemDescriptor<TData>> Deserialize(
    SerializedItemDescriptor item) {
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

FlagDescriptor JsonSource::GetFlag(std::string& key) const {
    // TODO: deserialize then return
    ISerializedDataSource::GetResult result = source_.Get(kind, key);
}
SegmentDescriptor JsonSource::GetSegment(std::string& key) const {
    // TODO: deserialize then return
    ISerializedDataSource::GetResult result = source_.Get(kind, key);
}
std::unordered_map<std::string, FlagDescriptor> JsonSource::AllFlags() const {
    // TODO: deserialize then return
    ISerializedDataSource::GetResult result = source_.All(kind);
}
std::unordered_map<std::string, SegmentDescriptor> JsonSource::AllSegments()
    const {
    // TODO: deserialize then return

    ISerializedDataSource::GetResult result = source_.All(kind);
}

std::string JsonSource::Identity() const {
    return source_.Identity();
}

}  // namespace launchdarkly::server_side::data::adapters
