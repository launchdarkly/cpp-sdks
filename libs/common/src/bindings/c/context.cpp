// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/bindings/c/context.h>
#include <launchdarkly/bindings/c/iter.hpp>
#include <launchdarkly/context.hpp>

using launchdarkly::AttributeReference;
using launchdarkly::Context;
using launchdarkly::Value;

#define AS_CONTEXT(x) reinterpret_cast<Context*>(x)

#define AS_PRIVATE_ATTRIBUTES_ITERATOR(x) \
    reinterpret_cast<                     \
        IteratorBinding<AttributeReference::SetType::const_iterator>*>(x)

LD_EXPORT(void) LDContext_Free(LDContext context) {
    delete AS_CONTEXT(context);
}

LD_EXPORT(bool) LDContext_Valid(LDContext context) {
    return AS_CONTEXT(context)->valid();
}

LD_EXPORT(LDValue)
LDContext_Get(LDContext context, char const* kind, char const* ref) {
    return reinterpret_cast<LDValue>(
        const_cast<Value*>(&(AS_CONTEXT(context)->get(kind, ref))));
}

LD_EXPORT(char const*) LDContext_Errors(LDContext context) {
    return AS_CONTEXT(context)->errors().c_str();
}

LD_EXPORT(LDContext_PrivateAttributesIter)
LDContext_CreatePrivateAttributesIter(LDContext context, char const* kind) {
    auto* cpp_context = AS_CONTEXT(context);
    auto found = std::find(cpp_context->kinds().begin(),
                           cpp_context->kinds().end(), kind);
    if (found != cpp_context->kinds().end()) {
        auto& attributes = cpp_context->attributes(kind);
        return reinterpret_cast<LDContext_PrivateAttributesIter>(
            new IteratorBinding<AttributeReference::SetType::const_iterator>{
                attributes.private_attributes().begin(),
                attributes.private_attributes().end()});
    }

    return nullptr;
}

LD_EXPORT(void)
LDContext_DestroyPrivateAttributesIter(LDContext_PrivateAttributesIter iter) {
    delete AS_PRIVATE_ATTRIBUTES_ITERATOR(iter);
}

LD_EXPORT(void)
LDContext_PrivateAttributesIter_Next(LDContext_PrivateAttributesIter iter) {
    AS_PRIVATE_ATTRIBUTES_ITERATOR(iter)->Next();
}

LD_EXPORT(bool)
LDContext_PrivateAttributesIter_End(LDContext_PrivateAttributesIter iter) {
    return AS_PRIVATE_ATTRIBUTES_ITERATOR(iter)->End();
}

LD_EXPORT(char const*)
LDContext_PrivateAttributesIter_Value(LDContext_PrivateAttributesIter iter) {
    auto& redaction_name =
        AS_PRIVATE_ATTRIBUTES_ITERATOR(iter)->iter->redaction_name();
    return redaction_name.c_str();
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
