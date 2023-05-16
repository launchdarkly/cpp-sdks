#include "json_item_descriptor.hpp"

#include <launchdarkly/serialization/json_evaluation_result.hpp>

#include <boost/core/ignore_unused.hpp>

namespace launchdarkly::client_side {

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                ItemDescriptor const& descriptor) {
    boost::ignore_unused(unused);

    auto& obj = json_value.emplace_object();

    obj.emplace("version", descriptor.version);

    if (descriptor.flag) {
        auto result = boost::json::value_from(*descriptor.flag);
        obj.emplace("flag", std::move(result));
    }
}

}  // namespace launchdarkly::client_side
