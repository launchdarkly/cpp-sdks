#include <boost/core/ignore_unused.hpp>

#include "json_all_flags.hpp"

#include <launchdarkly/serialization/json_evaluation_result.hpp>

namespace launchdarkly::client_side {
// This tag_invoke needs to be in the same namespace as the
// ItemDescriptor.

static tl::expected<
    std::unordered_map<std::string, launchdarkly::client_side::ItemDescriptor>,
    JsonError>
tag_invoke(boost::json::value_to_tag<tl::expected<
               std::unordered_map<std::string,
                                  launchdarkly::client_side::ItemDescriptor>,
               JsonError>> const& unused,
           boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (!json_value.is_object()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    auto const& obj = json_value.as_object();
    std::unordered_map<std::string, launchdarkly::client_side::ItemDescriptor>
        descriptors;
    for (auto const& pair : obj) {
        auto eval_result =
            boost::json::value_to<tl::expected<EvaluationResult, JsonError>>(
                pair.value());
        if (!eval_result.has_value()) {
            return tl::unexpected(JsonError::kSchemaFailure);
        }
        descriptors.emplace(pair.key(),
                            launchdarkly::client_side::ItemDescriptor(
                                std::move(eval_result.value())));
    }
    return descriptors;
}

void tag_invoke(
    boost::json::value_from_tag const& unused,
    boost::json::value& json_value,
    std::unordered_map<std::string, std::shared_ptr<ItemDescriptor>> const&
        all_flags) {
    boost::ignore_unused(unused);

    auto& obj = json_value.emplace_object();
    for (auto descriptor : all_flags) {
        // Only serialize non-deleted flags.
        if (descriptor.second->flag) {
            auto eval_result_json =
                boost::json::value_from(*descriptor.second->flag);
            obj.emplace(descriptor.first, eval_result_json);
        }
    }
}

}  // namespace launchdarkly::client_side
