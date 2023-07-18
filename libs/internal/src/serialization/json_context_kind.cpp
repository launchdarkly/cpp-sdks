#include <launchdarkly/serialization/json_context_kind.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>

#include <boost/core/ignore_unused.hpp>

namespace launchdarkly {
tl::expected<std::optional<data_model::ContextKind>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::ContextKind>, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_STRING(json_value);
    auto const& str = json_value.as_string();

    if (str.empty()) {
        /* Empty string is not a valid context kind. */
        return tl::make_unexpected(JsonError::kSchemaFailure);
    }

    return data_model::ContextKind(str.c_str());
}
}  // namespace launchdarkly
