#include <launchdarkly/serialization/json_attributes.hpp>
#include <launchdarkly/serialization/json_value.hpp>

#include <boost/core/ignore_unused.hpp>

namespace launchdarkly {
void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                Attributes const& attributes) {
    boost::ignore_unused(unused);

    auto& obj = json_value.emplace_object();

    obj.emplace("key", attributes.Key());
    if (!attributes.Name().empty()) {
        obj.emplace("name", attributes.Name());
    }

    if (attributes.Anonymous()) {
        obj.emplace("anonymous", attributes.Anonymous());
    }

    for (auto const& attr : attributes.CustomAttributes().AsObject()) {
        if (!attr.second.IsNull()) {
            obj.emplace(attr.first, boost::json::value_from(attr.second));
        }
    }

    auto private_attributes = attributes.PrivateAttributes();
    if (!private_attributes.empty()) {
        obj.emplace("_meta", boost::json::object());
        auto& meta = obj.at("_meta").as_object();
        meta.emplace("privateAttributes", boost::json::array());
        auto& output_array = meta.at("privateAttributes").as_array();
        for (auto const& ref : private_attributes) {
            output_array.push_back(boost::json::value(ref.RedactionName()));
        }
    }
}
}  // namespace launchdarkly
