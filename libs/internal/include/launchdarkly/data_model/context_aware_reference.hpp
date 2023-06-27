#pragma once

#include <launchdarkly/attribute_reference.hpp>
#include <string>

namespace launchdarkly::data_model {
template <typename Fields>
struct ContextAwareReference {
    using fields = Fields;
    std::string contextKind;
    AttributeReference reference;
};

}  // namespace launchdarkly::data_model
