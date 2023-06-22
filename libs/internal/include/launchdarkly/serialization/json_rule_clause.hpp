#include <boost/json/fwd.hpp>
#include <launchdarkly/data_model/rule_clause.hpp>
#include <launchdarkly/serialization/json_errors.hpp>
#include <tl/expected.hpp>

namespace launchdarkly {
tl::expected<data_model::Clause, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<data_model::Clause, JsonError>> const& unused,
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

}  // namespace launchdarkly
