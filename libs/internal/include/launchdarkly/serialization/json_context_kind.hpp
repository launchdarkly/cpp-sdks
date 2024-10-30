#pragma once

#include <launchdarkly/data_model/context_kind.hpp>
#include <launchdarkly/detail/serialization/json_errors.hpp>

#include <boost/json.hpp>
#include <tl/expected.hpp>

#include <optional>

namespace launchdarkly {

tl::expected<std::optional<data_model::ContextKind>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::ContextKind>, JsonError>> const&
        unused,
    boost::json::value const& json_value);

// Serializers need to be in launchdarkly::data_model for ADL.
namespace data_model {

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::ContextKind const& segment);

}

}  // namespace launchdarkly
