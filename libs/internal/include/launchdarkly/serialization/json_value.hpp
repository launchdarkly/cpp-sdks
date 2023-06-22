#pragma once

#include <boost/json.hpp>

#include <launchdarkly/serialization/json_errors.hpp>
#include <launchdarkly/value.hpp>
#include <tl/expected.hpp>

namespace launchdarkly {
/**
 * Method used by boost::json for converting a boost::json::value into a
 * launchdarkly::Value.
 * @return A Value representation of the boost::json::value.
 */
tl::expected<std::optional<Value>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<Value>, JsonError>> const&,
    boost::json::value const&);

Value tag_invoke(boost::json::value_to_tag<Value> const&,
                 boost::json::value const&);
/**
 * Method used by boost::json for converting a launchdarkly::Value into a
 * boost::json::value.
 */
void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                Value const& ld_value);
}  // namespace launchdarkly
