#pragma once

#include <launchdarkly/data_model/sdk_data_set.hpp>
#include <launchdarkly/detail/serialization/json_errors.hpp>

#include <boost/json/fwd.hpp>

namespace launchdarkly {
tl::expected<std::optional<data_model::SDKDataSet>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::SDKDataSet>, JsonError>> const&
        unused,
    boost::json::value const& json_value);

}  // namespace launchdarkly
