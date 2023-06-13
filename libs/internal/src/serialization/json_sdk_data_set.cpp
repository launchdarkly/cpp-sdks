#include <launchdarkly/serialization/json_sdk_data_set.hpp>

#include <boost/core/ignore_unused.hpp>

namespace launchdarkly {
tl::expected<data_model::SDKDataSet, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<data_model::SDKDataSet, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
}
}  // namespace launchdarkly
