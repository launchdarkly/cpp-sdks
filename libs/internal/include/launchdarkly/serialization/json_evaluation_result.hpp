#pragma once

#include "tl/expected.hpp"

#include <boost/json.hpp>

#include <launchdarkly/data/evaluation_result.hpp>
#include "json_errors.hpp"

namespace launchdarkly {

tl::expected<std::optional<EvaluationResult>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<EvaluationResult>, JsonError>> const& unused,
    boost::json::value const& json_value);

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                EvaluationResult const& evaluation_result);

}  // namespace launchdarkly
