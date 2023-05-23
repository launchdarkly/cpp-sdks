// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/bindings/c/context_builder.h>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/detail/c_binding_helpers.hpp>

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
    return reinterpret_cast<LDContextBuilder>(new BindingContextBuilder());
}

LD_EXPORT(void) LDContextBuilder_Free(LDContextBuilder builder) {
    delete AS_BUILDER(builder);
}

LD_EXPORT(LDContext) LDContextBuilder_Build(LDContextBuilder builder) {
    LD_ASSERT_NOT_NULL(builder);

    auto built = AS_BUILDER(builder)->builder.Build();
    auto context = new Context(std::move(built));
    LDContextBuilder_Free(builder);
    return reinterpret_cast<LDContext>(context);
}

LD_EXPORT(void)
LDContextBuilder_AddKind(LDContextBuilder builder,
                         char const* kind,
                         char const* key) {
    LD_ASSERT_NOT_NULL(builder);
    LD_ASSERT_NOT_NULL(kind);
    LD_ASSERT_NOT_NULL(key);

    auto* binding = AS_BUILDER(builder);
    binding->kind_to_key[kind] = key;
    binding->builder.Kind(kind, key);
}

LD_EXPORT(bool)
LDContextBuilder_Attributes_Set(LDContextBuilder builder,
                                char const* kind,
                                char const* attr_key,
                                LDValue val) {
    LD_ASSERT_NOT_NULL(builder);
    LD_ASSERT_NOT_NULL(kind);
    LD_ASSERT_NOT_NULL(attr_key);

    auto* binding = AS_BUILDER(builder);
    auto existing = binding->kind_to_key.find(kind);
    if (existing != binding->kind_to_key.end()) {
        auto& attributes = binding->builder.Kind(kind, existing->second);
        attributes.Set(attr_key, std::move(*AS_VALUE(val)));
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
    LD_ASSERT_NOT_NULL(builder);
    LD_ASSERT_NOT_NULL(kind);
    LD_ASSERT_NOT_NULL(attr_key);
    LD_ASSERT_NOT_NULL(val);

    auto* binding = AS_BUILDER(builder);
    auto existing = binding->kind_to_key.find(kind);
    if (existing != binding->kind_to_key.end()) {
        auto& attributes = binding->builder.Kind(kind, existing->second);
        attributes.SetPrivate(attr_key, std::move(*AS_VALUE(val)));
        delete AS_VALUE(val);

        return true;
    }
    return false;
}

LD_EXPORT(bool)
LDContextBuilder_Attributes_SetName(LDContextBuilder builder,
                                    char const* kind,
                                    char const* name) {
    LD_ASSERT_NOT_NULL(builder);
    LD_ASSERT_NOT_NULL(kind);
    LD_ASSERT_NOT_NULL(name);

    auto* binding = AS_BUILDER(builder);
    auto existing = binding->kind_to_key.find(kind);
    if (existing != binding->kind_to_key.end()) {
        auto& attributes = binding->builder.Kind(kind, existing->second);
        attributes.Name(name);

        return true;
    }
    return false;
}

LD_EXPORT(bool)
LDContextBuilder_Attributes_SetAnonymous(LDContextBuilder builder,
                                         char const* kind,
                                         bool anonymous) {
    LD_ASSERT_NOT_NULL(builder);
    LD_ASSERT_NOT_NULL(kind);

    auto* binding = AS_BUILDER(builder);
    auto existing = binding->kind_to_key.find(kind);
    if (existing != binding->kind_to_key.end()) {
        auto& attributes = binding->builder.Kind(kind, existing->second);
        attributes.Anonymous(anonymous);

        return true;
    }
    return false;
}

LD_EXPORT(bool)
LDContextBuilder_Attributes_AddPrivateAttribute(LDContextBuilder builder,
                                                char const* kind,
                                                char const* attr_key) {
    LD_ASSERT_NOT_NULL(builder);
    LD_ASSERT_NOT_NULL(kind);
    LD_ASSERT_NOT_NULL(attr_key);

    auto* binding = AS_BUILDER(builder);
    auto existing = binding->kind_to_key.find(kind);
    if (existing != binding->kind_to_key.end()) {
        auto& attributes = binding->builder.Kind(kind, existing->second);
        attributes.AddPrivateAttribute(attr_key);

        return true;
    }
    return false;
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
