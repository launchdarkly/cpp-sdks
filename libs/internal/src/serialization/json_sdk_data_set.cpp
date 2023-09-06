#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>
#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_item_descriptor.hpp>
#include <launchdarkly/serialization/json_sdk_data_set.hpp>
#include <launchdarkly/serialization/json_segment.hpp>
#include <tl/expected.hpp>

namespace launchdarkly {
tl::expected<std::optional<data_model::SDKDataSet>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::SDKDataSet>, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);

    auto const& obj = json_value.as_object();

    data_model::SDKDataSet data_set{};

    PARSE_FIELD(data_set.flags, obj, "flags");
    PARSE_FIELD(data_set.segments, obj, "segments");

    return data_set;
}
}  // namespace launchdarkly
