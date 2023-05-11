#include <launchdarkly/attributes.hpp>

namespace launchdarkly {

std::string const& Attributes::key() const {
    return key_.AsString();
}

std::string const& Attributes::name() const {
    return name_.AsString();
}

bool Attributes::anonymous() const {
    return anonymous_.AsBool();
}

Value const& Attributes::custom_attributes() const {
    return custom_attributes_;
}

AttributeReference::SetType const& Attributes::private_attributes() const {
    return private_attributes_;
}

}  // namespace launchdarkly
