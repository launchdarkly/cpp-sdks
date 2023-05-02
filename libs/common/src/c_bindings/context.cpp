// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include "c_bindings/context.h"
#include "context.hpp"

using launchdarkly::Context;
using launchdarkly::Value;

#define AS_CONTEXT(x) reinterpret_cast<Context*>(x)

LD_EXPORT(bool) valid(LDContext context) {
    return AS_CONTEXT(context)->valid();
}

LD_EXPORT(LDValue) get(LDContext context, char const* kind, char const* ref) {
    return const_cast<Value*>(&(AS_CONTEXT(context)->get(kind, ref)));
}

LD_EXPORT(char const*) errors(LDContext context) {
    return AS_CONTEXT(context)->errors().c_str();
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection