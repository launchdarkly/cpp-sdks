#pragma once

#include <tl/expected.hpp>

#include <boost/json.hpp>

#include "data/evaluation_result.hpp"
#include "serialization/json_errors.hpp"

namespace launchdarkly {
/**
 * Method used by boost::json for converting a boost::json::value into a
 * launchdarkly::EvaluationResult.
 * @return A EvaluationResult representation of the boost::json::value.
 */
tl::expected<EvaluationResult, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<EvaluationResult, JsonError>> const&
        unused,
    boost::json::value const& json_value);
}  // namespace launchdarkly
