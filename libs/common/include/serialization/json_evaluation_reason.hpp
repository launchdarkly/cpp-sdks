#pragma once

#include <boost/json.hpp>

#include "data/evaluation_reason.hpp"

namespace launchdarkly {
/**
 * Method used by boost::json for converting a boost::json::value into a
 * launchdarkly::EvaluationReason.
 * @return A EvaluationReason representation of the boost::json::value.
 */
EvaluationReason tag_invoke(
    boost::json::value_to_tag<EvaluationReason> const& unused,
    boost::json::value const& json_value);
}  // namespace launchdarkly
