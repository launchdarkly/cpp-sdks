#pragma once

#include <boost/json.hpp>

#include <tl/expected.hpp>

#include <launchdarkly/context.hpp>
#include <launchdarkly/serialization/json_errors.hpp>

namespace launchdarkly {
/**
 * Method used by boost::json for converting a launchdarkly::Context into a
 * boost::json::value.
 */
void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                Context const& ld_context);

/**
 * Method used by boost::json for converting JSON into a launchdarkly::Context.
 */
tl::expected<Context, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<Context, JsonError>> const& unused,
    boost::json::value const& json_value);
}  // namespace launchdarkly
