#include "launchdarkly/attributes.hpp"

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

}  // namespace launchdarkly
