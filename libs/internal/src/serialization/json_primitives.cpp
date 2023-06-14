#include <launchdarkly/serialization/json_primitives.hpp>

namespace launchdarkly {
tl::expected<bool, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<bool, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (!json_value.is_bool()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    return json_value.as_bool();
}

tl::expected<std::uint64_t, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::uint64_t, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    if (!json_value.is_number()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    return json_value.to_number<uint64_t>();
}

tl::expected<std::string, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::string, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    
    if (!json_value.is_string()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    if (json_value.as_string().empty()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    return std::string(json_value.as_string());
}
}  // namespace launchdarkly
