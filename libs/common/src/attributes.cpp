#include "attributes.hpp"

#include <boost/json.hpp>

namespace launchdarkly {

std::string const& Attributes::key() const {
    return key_.as_string();
}

std::string const& Attributes::name() const {
    return name_.as_string();
}

bool Attributes::anonymous() const {
    return anonymous_.as_bool();
}

Value const& Attributes::custom_attributes() const {
    return custom_attributes_;
}

AttributeReference::SetType const& Attributes::private_attributes() const {
    return private_attributes_;
}

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                Attributes const& attributes) {
    auto& obj = json_value.emplace_object();

    obj.emplace("key", attributes.key());
    if(!attributes.name().empty()) {
        obj.emplace("name", attributes.name());
    }
    if(attributes.anonymous()) {
        obj.emplace("anonymous", attributes.anonymous());
    }
    for(auto const& attr: attributes.custom_attributes().as_object()) {
        obj.emplace(attr.first, boost::json::value_from(attr.second));
    }
}

}  // namespace launchdarkly
