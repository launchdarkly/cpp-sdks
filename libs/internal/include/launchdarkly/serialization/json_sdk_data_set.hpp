#pragma once

#include <boost/json/fwd.hpp>
#include <launchdarkly/data_model/sdk_data_set.hpp>
#include <launchdarkly/serialization/json_errors.hpp>

namespace launchdarkly {
tl::expected<data_model::SDKDataSet, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<data_model::SDKDataSet, JsonError>> const& unused,
    boost::json::value const& json_value);

}
