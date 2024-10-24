#pragma once

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/detail/serialization/json_errors.hpp>

#include <boost/json/fwd.hpp>

namespace launchdarkly {

tl::expected<std::optional<data_model::Flag::Rollout>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Flag::Rollout>,
                     JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::optional<data_model::Flag::VariationOrRollout>, JsonError>
tag_invoke(boost::json::value_to_tag<
               tl::expected<std::optional<data_model::Flag::VariationOrRollout>,
                            JsonError>> const& unused,
           boost::json::value const& json_value);

tl::expected<std::optional<data_model::Flag::Rollout::WeightedVariation>,
             JsonError>
tag_invoke(boost::json::value_to_tag<tl::expected<
               std::optional<data_model::Flag::Rollout::WeightedVariation>,
               JsonError>> const& unused,
           boost::json::value const& json_value);

tl::expected<std::optional<data_model::Flag::Rollout::Kind>, JsonError>
tag_invoke(boost::json::value_to_tag<
               tl::expected<std::optional<data_model::Flag::Rollout::Kind>,
                            JsonError>> const& unused,
           boost::json::value const& json_value);

tl::expected<std::optional<data_model::Flag::Prerequisite>, JsonError>
tag_invoke(boost::json::value_to_tag<
               tl::expected<std::optional<data_model::Flag::Prerequisite>,
                            JsonError>> const& unused,
           boost::json::value const& json_value);

tl::expected<std::optional<data_model::Flag::Target>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Flag::Target>, JsonError>> const&
        unused,
    boost::json::value const& json_value);

tl::expected<std::optional<data_model::Flag::Rule>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Flag::Rule>, JsonError>> const&
        unused,
    boost::json::value const& json_value);

tl::expected<std::optional<data_model::Flag::ClientSideAvailability>, JsonError>
tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Flag::ClientSideAvailability>,
                     JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::optional<data_model::Flag>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Flag>, JsonError>> const& unused,
    boost::json::value const& json_value);

// Serializers need to be in launchdarkly::data_model for ADL.
namespace data_model {

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::Flag::Rollout const& rollout);

void tag_invoke(
    boost::json::value_from_tag const& unused,
    boost::json::value& json_value,
    data_model::Flag::VariationOrRollout const& variation_or_rollout);

void tag_invoke(
    boost::json::value_from_tag const& unused,
    boost::json::value& json_value,
    data_model::Flag::Rollout::WeightedVariation const& weighted_variation);

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::Flag::Rollout::Kind const& kind);

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::Flag::Prerequisite const& prerequisite);

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::Flag::Target const& target);

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::Flag::Rule const& rule);

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::Flag::ClientSideAvailability const& availability);

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::Flag const& flag);

}  // namespace data_model
}  // namespace launchdarkly
