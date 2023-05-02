// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include "c_bindings/context_builder.h"
#include "context_builder.hpp"
#include "value.hpp"

#define AS_BUILDER(x) reinterpret_cast<BindingContextBuilder*>(x)

#define AS_VALUE(x) reinterpret_cast<Value*>(x)

using launchdarkly::Context;
using launchdarkly::ContextBuilder;
using launchdarkly::Value;

struct BindingContextBuilder {
    std::map<std::string, std::string> kind_to_key;
    ContextBuilder builder;
};

LD_EXPORT(LDContextBuilder) LDContextBuilder_New() {
    return new BindingContextBuilder();
}

LD_EXPORT(void) LDContextBuilder_Free(LDContextBuilder builder) {
    delete AS_BUILDER(builder);
}

LD_EXPORT(LDContext) LDContextBuilder_Build(LDContextBuilder builder) {
    auto built = AS_BUILDER(builder)->builder.build();
    return new Context(built);
}

LD_EXPORT(void)
LDContextBuilder_AddKind(LDContextBuilder builder,
                         char const* kind,
                         char const* key) {
    auto* binding = AS_BUILDER(builder);
    binding->builder.kind(kind, key);
}

LD_EXPORT(bool)
LDContextBuilder_Attributes_Set(LDContextBuilder builder,
                                char const* kind,
                                char const* attr_key,
                                LDValue val) {
    auto* binding = AS_BUILDER(builder);
    auto existing = binding->kind_to_key.find(kind);
    if (existing != binding->kind_to_key.end()) {
        auto& attributes = binding->builder.kind(kind, existing->first);
        attributes.set(attr_key, std::move(*AS_VALUE(val)));
        delete AS_VALUE(val);

        return true;
    }
    return false;
}

LD_EXPORT(bool)
LDContextBuilder_Attributes_SetPrivate(LDContextBuilder builder,
                                       char const* kind,
                                       char const* attr_key,
                                       LDValue val) {
    auto* binding = AS_BUILDER(builder);
    auto existing = binding->kind_to_key.find(kind);
    if (existing != binding->kind_to_key.end()) {
        auto& attributes = binding->builder.kind(kind, existing->first);
        attributes.set_private(attr_key, std::move(*AS_VALUE(val)));
        delete AS_VALUE(val);

        return true;
    }
    return false;
}

LD_EXPORT(bool)
LDContextBuilder_Attributes_SetName(LDContextBuilder builder,
                                    char const* kind,
                                    char const* name) {
    auto* binding = AS_BUILDER(builder);
    auto existing = binding->kind_to_key.find(kind);
    if (existing != binding->kind_to_key.end()) {
        auto& attributes = binding->builder.kind(kind, existing->first);
        attributes.name(name);

        return true;
    }
    return false;
}

LD_EXPORT(bool)
LDContextBuilder_Attributes_SetAnonymous(LDContextBuilder builder,
                                         char const* kind,
                                         bool anonymous) {
    auto* binding = AS_BUILDER(builder);
    auto existing = binding->kind_to_key.find(kind);
    if (existing != binding->kind_to_key.end()) {
        auto& attributes = binding->builder.kind(kind, existing->first);
        attributes.anonymous(anonymous);

        return true;
    }
    return false;
}

LD_EXPORT(bool)
LDContextBuilder_Attributes_AddPrivateAttribute(LDContextBuilder builder,
                                                char const* kind,
                                                char const* attr_key) {
    auto* binding = AS_BUILDER(builder);
    auto existing = binding->kind_to_key.find(kind);
    if (existing != binding->kind_to_key.end()) {
        auto& attributes = binding->builder.kind(kind, existing->first);
        attributes.add_private_attribute(attr_key);

        return true;
    }
    return false;
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
