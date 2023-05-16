#include <boost/core/ignore_unused.hpp>

#include "json_all_flags.hpp"

#include <launchdarkly/serialization/json_evaluation_result.hpp>

namespace launchdarkly::client_side::serialization {

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

}  // namespace launchdarkly::client_side::serialization
