#pragma once

#include <boost/json.hpp>

#include "../data_sources/data_source_update_sink.hpp"

namespace launchdarkly::client_side {
/**
 * Method used by boost::json for converting launchdarkly::client_side::ItemDescriptor into a
 * boost::json::value.
 */
void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                ItemDescriptor const& descriptor);
}  // namespace launchdarkly::client_side
