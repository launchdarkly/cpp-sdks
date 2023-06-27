#pragma once

#include <boost/json/fwd.hpp>
#include <launchdarkly/data_model/segment.hpp>
#include <launchdarkly/serialization/json_errors.hpp>

namespace launchdarkly {
tl::expected<std::optional<data_model::Segment>, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::optional<data_model::Segment>,
                                           JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::optional<data_model::Segment::Target>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Segment::Target>,
                     JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::optional<data_model::Segment::Rule>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Segment::Rule>,
                     JsonError>> const& unused,
    boost::json::value const& json_value);

}  // namespace launchdarkly
