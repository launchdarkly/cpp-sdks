#include "serialization/json_attributes.hpp"
#include "serialization/json_value.hpp"

namespace launchdarkly {
void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                Attributes const& attributes) {
    boost::ignore_unused(unused);

    auto& obj = json_value.emplace_object();

    obj.emplace("key", attributes.key());
    if (!attributes.name().empty()) {
        obj.emplace("name", attributes.name());
    }

    if (attributes.anonymous()) {
        obj.emplace("anonymous", attributes.anonymous());
    }

    for (auto const& attr : attributes.custom_attributes().AsObject()) {
        obj.emplace(attr.first, boost::json::value_from(attr.second));
    }

    auto private_attributes = attributes.private_attributes();
    if (!private_attributes.empty()) {
        obj.emplace("_meta", boost::json::object());
        auto& meta = obj.at("_meta").as_object();
        meta.emplace("privateAttributes", boost::json::array());
        auto& output_array = meta.at("privateAttributes").as_array();
        for (auto const& ref : private_attributes) {
            output_array.push_back(boost::json::value(ref.redaction_name()));
        }
    }
}
}  // namespace launchdarkly
