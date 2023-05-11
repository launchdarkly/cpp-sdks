#include "launchdarkly/attributes_builder.hpp"
#include "launchdarkly/context_builder.hpp"

namespace launchdarkly {

template <>
AttributesBuilder<ContextBuilder, Context>&
AttributesBuilder<ContextBuilder, Context>::name(std::string name) {
    name_ = std::move(name);
    return *this;
}

template <>
AttributesBuilder<ContextBuilder, Context>&
AttributesBuilder<ContextBuilder, Context>::anonymous(bool anonymous) {
    anonymous_ = anonymous;
    return *this;
}

template <>
AttributesBuilder<ContextBuilder, Context>&
AttributesBuilder<ContextBuilder, Context>::set(std::string name,
                                                Value value,
                                                bool private_attribute) {
    if (name == "key" || name == "kind" || name == "anonymous" ||
        name == "name") {
        return *this;
    }

    if (private_attribute) {
        this->private_attributes_.insert(name);
    }
    values_[std::move(name)] = std::move(value);
    return *this;
}

template <>
AttributesBuilder<ContextBuilder, Context>&
AttributesBuilder<ContextBuilder, Context>::set(std::string name, Value value) {
    return set(std::move(name), std::move(value), false);
}

template <>
AttributesBuilder<ContextBuilder, Context>&
AttributesBuilder<ContextBuilder, Context>::set_private(std::string name,
                                                        Value value) {
    return set(std::move(name), std::move(value), true);
}

template <>
AttributesBuilder<ContextBuilder, Context>&
AttributesBuilder<ContextBuilder, Context>::add_private_attribute(
    AttributeReference ref) {
    private_attributes_.insert(std::move(ref));
    return *this;
}

template <>
Attributes AttributesBuilder<ContextBuilder, Context>::build_attributes() {
    return {std::move(key_), std::move(name_), anonymous_, std::move(values_),
            std::move(private_attributes_)};
}

}  // namespace launchdarkly
