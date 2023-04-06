#pragma once

#include <boost/json.hpp>

#include "context.hpp"

namespace launchdarkly {
/**
 * Method used by boost::json for converting a launchdarkly::Context into a
 * boost::json::value.
 */
void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                Context const& ld_context);
}  // namespace launchdarkly
