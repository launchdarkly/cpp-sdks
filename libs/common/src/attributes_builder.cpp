#include <launchdarkly/attributes_builder.hpp>
#include <launchdarkly/context_builder.hpp>

namespace launchdarkly {

template <>
AttributesBuilder<ContextBuilder, Context>&
AttributesBuilder<ContextBuilder, Context>::Name(std::string name) {
    name_ = std::move(name);
    return *this;
}

template <>
AttributesBuilder<ContextBuilder, Context>&
AttributesBuilder<ContextBuilder, Context>::Anonymous(bool anonymous) {
    anonymous_ = anonymous;
    return *this;
}

template <>
AttributesBuilder<ContextBuilder, Context>&
AttributesBuilder<ContextBuilder, Context>::Set(std::string name,
                                                Value value,
                                                bool private_attribute) {
    if (name == "key" || name == "kind" || name == "anonymous" ||
        name == "name" || name == "_meta") {
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
AttributesBuilder<ContextBuilder, Context>::Set(std::string name, Value value) {
    return Set(std::move(name), std::move(value), false);
}

template <>
AttributesBuilder<ContextBuilder, Context>&
AttributesBuilder<ContextBuilder, Context>::SetPrivate(std::string name,
                                                        Value value) {
    return Set(std::move(name), std::move(value), true);
}

template <>
AttributesBuilder<ContextBuilder, Context>&
AttributesBuilder<ContextBuilder, Context>::AddPrivateAttribute(
    AttributeReference ref) {
    private_attributes_.insert(std::move(ref));
    return *this;
}

template <>
Attributes AttributesBuilder<ContextBuilder, Context>::BuildAttributes() {
    return {std::move(key_), std::move(name_), anonymous_, std::move(values_),
            std::move(private_attributes_)};
}

}  // namespace launchdarkly
