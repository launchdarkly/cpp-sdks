#pragma once

#include <launchdarkly/attributes.hpp>

#include <boost/json/fwd.hpp>

namespace launchdarkly {
/**
 * Method used by boost::json for converting launchdarkly::Attributes into a
 * boost::json::value.
 */
void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                Attributes const& attributes);
}  // namespace launchdarkly
