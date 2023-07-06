#pragma once

#include <launchdarkly/data/evaluation_reason.hpp>
#include <launchdarkly/serialization/json_errors.hpp>

#include <boost/json/fwd.hpp>
#include <tl/expected.hpp>

namespace launchdarkly {
/**
 * Method used by boost::json for converting a boost::json::value into a
 * launchdarkly::EvaluationReason.
 * @return A EvaluationReason representation of the boost::json::value.
 */
tl::expected<EvaluationReason, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<EvaluationReason, JsonError>> const&
        unused,
    boost::json::value const& json_value);

tl::expected<enum EvaluationReason::Kind, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<enum EvaluationReason::Kind, JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<enum EvaluationReason::ErrorKind, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<enum EvaluationReason::ErrorKind,
                                           JsonError>> const& unused,
    boost::json::value const& json_value);

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                enum EvaluationReason::Kind const& kind);

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                enum EvaluationReason::ErrorKind const& kind);

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                EvaluationReason const& reason);
}  // namespace launchdarkly
