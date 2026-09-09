#pragma once

#include <launchdarkly/data/evaluation_result.hpp>
#include <launchdarkly/detail/serialization/json_errors.hpp>

#include <boost/json/fwd.hpp>
#include <tl/expected.hpp>

#include <cstdint>
#include <optional>

namespace launchdarkly {

/**
 * Deserializes an evaluation result.
 *
 * @param json_value The object to deserialize. A null value yields
 * std::nullopt rather than an error.
 * @param version_override The version to assign to the result. Pass
 * std::nullopt to read the version from the object itself.
 */
tl::expected<std::optional<EvaluationResult>, JsonError> ParseEvaluationResult(
    boost::json::value const& json_value,
    std::optional<std::uint64_t> version_override);

tl::expected<std::optional<EvaluationResult>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<EvaluationResult>, JsonError>> const& unused,
    boost::json::value const& json_value);

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                EvaluationResult const& evaluation_result);

}  // namespace launchdarkly
