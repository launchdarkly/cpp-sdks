#pragma once

#include <launchdarkly/data_model/rule_clause.hpp>
#include <launchdarkly/detail/serialization/json_errors.hpp>

#include <boost/json/fwd.hpp>
#include <tl/expected.hpp>

namespace launchdarkly {
tl::expected<std::optional<data_model::Clause>, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::optional<data_model::Clause>,
                                           JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::optional<data_model::Clause::Op>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Clause::Op>, JsonError>> const&
        unused,
    boost::json::value const& json_value);

tl::expected<data_model::Clause::Op, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<data_model::Clause::Op, JsonError>> const& unused,
    boost::json::value const& json_value);

// Serialization needs to be in launchdarkly::data_model for ADL.
namespace data_model {

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::Clause const& clause);

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::Clause::Op const& op);

}  // namespace data_model

}  // namespace launchdarkly
