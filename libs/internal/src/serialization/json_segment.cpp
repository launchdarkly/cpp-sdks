#include <boost/core/ignore_unused.hpp>
#include <launchdarkly/serialization/json_segment.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>

namespace launchdarkly {

tl::expected<data_model::Segment::Target, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<data_model::Segment::Target, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);

    auto const& obj = json_value.as_object();

    data_model::Segment::Target target;

    PARSE_REQUIRED_FIELD(target.contextKind, obj, "contextKind");
    PARSE_REQUIRED_FIELD(target.values, obj, "values");

    return target;
}

tl::expected<data_model::Segment, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<data_model::Segment, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);

    auto const& obj = json_value.as_object();

    data_model::Segment segment;

    PARSE_REQUIRED_FIELD(segment.key, obj, "key");
    PARSE_REQUIRED_FIELD(segment.version, obj, "version");

    PARSE_OPTIONAL_FIELD(segment.excluded, obj, "excluded");
    PARSE_OPTIONAL_FIELD(segment.included, obj, "included");

    PARSE_OPTIONAL_FIELD(segment.generation, obj, "generation");
    PARSE_OPTIONAL_FIELD(segment.salt, obj, "salt");
    PARSE_OPTIONAL_FIELD(segment.unbounded, obj, "unbounded");

    PARSE_OPTIONAL_FIELD(segment.includedContexts, obj, "includedContexts");
    PARSE_OPTIONAL_FIELD(segment.excludedContexts, obj, "excludedContexts");

    return segment;
}
}  // namespace launchdarkly
