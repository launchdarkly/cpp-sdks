#include <launchdarkly/serialization/json_primitives.hpp>

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
    if (!json_value.as_bool()) {
        return std::nullopt;
    }
    return json_value.as_bool();
}

tl::expected<bool, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<bool, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    auto maybe_val =
        boost::json::value_to<tl::expected<std::optional<bool>, JsonError>>(
            json_value);
    if (!maybe_val.has_value()) {
        return tl::unexpected(maybe_val.error());
    }
    return maybe_val.value().value_or(false);
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
    return json_value.to_number<uint64_t>();
}

tl::expected<std::uint64_t, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::uint64_t, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    auto maybe_val = boost::json::value_to<
        tl::expected<std::optional<std::uint64_t>, JsonError>>(json_value);
    if (!maybe_val.has_value()) {
        return tl::unexpected(maybe_val.error());
    }
    return maybe_val.value().value_or(0);
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
    if (json_value.as_string().empty()) {
        return std::nullopt;
    }
    return std::string(json_value.as_string());
}

tl::expected<std::string, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::string, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    auto maybe_val = boost::json::value_to<
        tl::expected<std::optional<std::string>, JsonError>>(json_value);
    if (!maybe_val.has_value()) {
        return tl::unexpected(maybe_val.error());
    }
    return maybe_val.value().value_or("");
}
}  // namespace launchdarkly
