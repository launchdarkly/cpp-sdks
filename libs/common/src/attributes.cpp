#include <launchdarkly/attributes.hpp>

namespace launchdarkly {

std::string const& Attributes::Key() const {
    return key_.AsString();
}

std::string const& Attributes::Name() const {
    return name_.AsString();
}

bool Attributes::Anonymous() const {
    return anonymous_.AsBool();
}

Value const& Attributes::CustomAttributes() const {
    return custom_attributes_;
}

AttributeReference::SetType const& Attributes::PrivateAttributes() const {
    return private_attributes_;
}

}  // namespace launchdarkly
