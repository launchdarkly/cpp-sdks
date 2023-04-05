#pragma once

#include <boost/json.hpp>

#include "data/evaluation_result.hpp"

namespace launchdarkly {
/**
 * Method used by boost::json for converting a boost::json::value into a
 * launchdarkly::EvaluationResult.
 * @return A EvaluationResult representation of the boost::json::value.
 */
EvaluationResult tag_invoke(
    boost::json::value_to_tag<EvaluationResult> const& unused,
    boost::json::value const& json_value);
}  // namespace launchdarkly
