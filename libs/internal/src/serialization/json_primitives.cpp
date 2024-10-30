#include <launchdarkly/detail/serialization/json_primitives.hpp>

namespace launchdarkly {
tl::expected<std::optional<bool>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<bool>, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    if (json_value.is_null()) {
        return std::nullopt;
    }
    if (!json_value.is_bool()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    return json_value.as_bool();
}

tl::expected<std::optional<std::uint64_t>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<std::uint64_t>, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    if (json_value.is_null()) {
        return std::nullopt;
    }
    if (!json_value.is_number()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    boost::system::error_code ec;
    auto val = json_value.to_number<uint64_t>(ec);
    if (ec) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    return val;
}

tl::expected<std::optional<std::int64_t>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<std::int64_t>, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    if (json_value.is_null()) {
        return std::nullopt;
    }
    if (!json_value.is_number()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    boost::system::error_code ec;
    auto val = json_value.to_number<int64_t>(ec);
    if (ec) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    return val;
}

tl::expected<std::optional<std::string>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<std::string>, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    if (json_value.is_null()) {
        return std::nullopt;
    }
    if (!json_value.is_string()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    return std::string(json_value.as_string());
}
}  // namespace launchdarkly
