#include <boost/json/fwd.hpp>
#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/serialization/json_errors.hpp>

namespace launchdarkly {
tl::expected<std::optional<data_model::Flag>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Flag>, JsonError>> const& unused,
    boost::json::value const& json_value);

}  // namespace launchdarkly
