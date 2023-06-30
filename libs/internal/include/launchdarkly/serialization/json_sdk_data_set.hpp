#pragma once

#include <launchdarkly/data_model/sdk_data_set.hpp>

namespace launchdarkly {
tl::expected<data_model::SDKDataSet, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<data_model::SDKDataSet, JsonError>> const& unused,
    boost::json::value const& json_value);

}
