// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/bindings/c/context.h>
#include <launchdarkly/bindings/c/iter.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/detail/c_binding_helpers.hpp>

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
    LD_ASSERT_NOT_NULL(context);

    return AS_CONTEXT(context)->Valid();
}

LD_EXPORT(LDValue)
LDContext_Get(LDContext context, char const* kind, char const* ref) {
    LD_ASSERT_NOT_NULL(context);
    LD_ASSERT_NOT_NULL(kind);
    LD_ASSERT_NOT_NULL(ref);

    return reinterpret_cast<LDValue>(
        const_cast<Value*>(&(AS_CONTEXT(context)->Get(kind, ref))));
}

LD_EXPORT(char const*) LDContext_Errors(LDContext context) {
    LD_ASSERT_NOT_NULL(context);

    return AS_CONTEXT(context)->errors().c_str();
}

LD_EXPORT(LDContext_PrivateAttributesIter)
LDContext_PrivateAttributesIter_New(LDContext context, char const* kind) {
    LD_ASSERT_NOT_NULL(context);
    LD_ASSERT_NOT_NULL(kind);

    auto* cpp_context = AS_CONTEXT(context);
    auto found = std::find(cpp_context->Kinds().begin(),
                           cpp_context->Kinds().end(), kind);
    if (found != cpp_context->Kinds().end()) {
        auto& attributes = cpp_context->Attributes(kind);
        return reinterpret_cast<LDContext_PrivateAttributesIter>(
            new IteratorBinding<AttributeReference::SetType::const_iterator>{
                attributes.PrivateAttributes().begin(),
                attributes.PrivateAttributes().end()});
    }

    return nullptr;
}

LD_EXPORT(void)
LDContext_PrivateAttributesIter_Free(LDContext_PrivateAttributesIter iter) {
    delete AS_PRIVATE_ATTRIBUTES_ITERATOR(iter);
}

LD_EXPORT(void)
LDContext_PrivateAttributesIter_Next(LDContext_PrivateAttributesIter iter) {
    LD_ASSERT_NOT_NULL(iter);

    AS_PRIVATE_ATTRIBUTES_ITERATOR(iter)->Next();
}

LD_EXPORT(bool)
LDContext_PrivateAttributesIter_End(LDContext_PrivateAttributesIter iter) {
    LD_ASSERT_NOT_NULL(iter);

    return AS_PRIVATE_ATTRIBUTES_ITERATOR(iter)->End();
}

LD_EXPORT(char const*)
LDContext_PrivateAttributesIter_Value(LDContext_PrivateAttributesIter iter) {
    LD_ASSERT_NOT_NULL(iter);

    auto& redaction_name =
        AS_PRIVATE_ATTRIBUTES_ITERATOR(iter)->iter->RedactionName();
    return redaction_name.c_str();
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
