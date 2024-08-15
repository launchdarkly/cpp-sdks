#include <launchdarkly/attributes_builder.hpp>
#include <launchdarkly/context_builder.hpp>

#include <set>
#include <unordered_map>
#include <functional>

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

// These attributes values cannot be set via the public Set/SetPrivate methods.
// 'key' and 'kind' are defined in the AttributesBuilder constructor, whereas
// '_meta' is set internally by the SDK.
std::set<std::string> const& ProtectedAttributes() {
    static std::set<std::string> protectedAttrs = {
        "key",
        "kind",
        "_meta"
    };
    return protectedAttrs;
}

// Utility struct to enforce the type-safe setting of a particular attribute
// via a method on AttributesBuilder.
struct TypedAttribute {
    enum Value::Type type;
    std::function<void(AttributesBuilder<ContextBuilder, Context>& builder,
                       Value)> setter;

    TypedAttribute(enum Value::Type type,
                   std::function<void(
                       AttributesBuilder<ContextBuilder, Context>& builder,
                       Value const&)> setter)
        : type(type), setter(std::move(setter)) {
    }
};

// These attribute values, while able to be set via public Set/SetPrivate methods,
// are regarded as special and are type-enforced. Instead of being stored in the
// internal AttributesBuilder::values map, they are stored as individual fields.
// This is because they have special meaning/usage within the LaunchDarkly Platform.
std::unordered_map<std::string, TypedAttribute> const&
TypedAttributes() {
    static std::unordered_map<std::string, TypedAttribute> typedAttrs = {
        {
            "anonymous", {Value::Type::kBool,
                          [](AttributesBuilder<
                                 ContextBuilder, Context>
                             & builder,
                             Value const& value) {
                              builder.Anonymous(
                                  value.AsBool());
                          }}
        },
        {"name", {Value::Type::kString,
                  [](AttributesBuilder<
                         ContextBuilder, Context>&
                     builder,
                     Value const& value) {
                      builder.Name(value.AsString());
                  }}}
    };
    return typedAttrs;
}


template <>
AttributesBuilder<ContextBuilder, Context>&
AttributesBuilder<ContextBuilder, Context>::Set(std::string name,
                                                Value value,
                                                bool private_attribute) {
    // Protected attributes cannot be set at all.
    if (ProtectedAttributes().count(name) > 0) {
        return *this;
    }

    // Typed attributes can be set only if the value is of the correct type;
    // the setting is forwarded to one of AttributeBuilder's dedicated methods.
    auto const typed_attr = TypedAttributes().find(name);
    const bool is_typed = typed_attr != TypedAttributes().end();
    if (is_typed && value.Type() != typed_attr->second.type) {
        return *this;
    }

    if (is_typed) {
        typed_attr->second.setter(*this, value);
    } else {
        values_[name] = std::move(value);
    }

    if (private_attribute) {
        this->private_attributes_.insert(std::move(name));
    }
    return *this;
}

template <>
AttributesBuilder<ContextBuilder, Context>&
AttributesBuilder<ContextBuilder, Context>::Set(
    std::string name,
    Value value) {
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
Attributes AttributesBuilder<
    ContextBuilder, Context>::BuildAttributes() const {
    return {key_, name_, anonymous_, values_, private_attributes_};
}
} // namespace launchdarkly
