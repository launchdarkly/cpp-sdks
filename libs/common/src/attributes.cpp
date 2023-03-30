#include "attributes.hpp"

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
}  // namespace launchdarkly
