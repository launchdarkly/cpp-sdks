#pragma once

#include <launchdarkly/data_model/segment.hpp>
#include <launchdarkly/serialization/json_errors.hpp>

#include <boost/json/fwd.hpp>

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

// Serializers need to be in launchdarkly::data_model for ADL.
namespace data_model {

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::Segment const& segment);

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::Segment::Target const& target);

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::Segment::Rule const& rule);

}  // namespace data_model

}  // namespace launchdarkly
