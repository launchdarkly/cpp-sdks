#pragma once

#include <tl/expected.hpp>

#include <boost/json.hpp>

#include "data/evaluation_reason.hpp"
#include "serialization/json_errors.hpp"

namespace launchdarkly {
/**
 * Method used by boost::json for converting a boost::json::value into a
 * launchdarkly::EvaluationReason.
 * @return A EvaluationReason representation of the boost::json::value.
 */
tl::expected<EvaluationReason, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<EvaluationReason, JsonError>> const& unused,
    boost::json::value const& json_value);
}  // namespace launchdarkly
