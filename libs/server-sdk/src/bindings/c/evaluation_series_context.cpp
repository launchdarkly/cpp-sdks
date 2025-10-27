#include <launchdarkly/server_side/bindings/c/evaluation_series_context.h>
#include "context_wrappers.hpp"

#include <launchdarkly/bindings/c/context.h>
#include <launchdarkly/bindings/c/value.h>
#include <launchdarkly/detail/c_binding_helpers.hpp>

#define AS_WRAPPER(ptr) \
    (reinterpret_cast<launchdarkly::server_side::bindings::EvaluationSeriesContextWrapper const*>(ptr))

#define AS_CONTEXT(ptr) \
    (reinterpret_cast<LDContext>(const_cast<launchdarkly::Context*>(ptr)))

#define AS_VALUE(ptr) \
    (reinterpret_cast<LDValue>(const_cast<launchdarkly::Value*>(ptr)))

#define AS_HOOK_CONTEXT(ptr) \
    (reinterpret_cast<LDHookContext>(const_cast<launchdarkly::server_side::hooks::HookContext*>(ptr)))

LD_EXPORT(char const*)
LDEvaluationSeriesContext_FlagKey(LDServerSDKEvaluationSeriesContext eval_context) {
    LD_ASSERT(eval_context != nullptr);
    return AS_WRAPPER(eval_context)->context.FlagKey().data();
}

LD_EXPORT(LDContext)
LDEvaluationSeriesContext_Context(LDServerSDKEvaluationSeriesContext eval_context) {
    LD_ASSERT(eval_context != nullptr);
    return AS_CONTEXT(&AS_WRAPPER(eval_context)->context.EvaluationContext());
}

LD_EXPORT(LDValue)
LDEvaluationSeriesContext_DefaultValue(
    LDServerSDKEvaluationSeriesContext eval_context) {
    LD_ASSERT(eval_context != nullptr);
    // Return pointer to the value in the C++ context (no copy needed)
    return AS_VALUE(&AS_WRAPPER(eval_context)->context.DefaultValue());
}

LD_EXPORT(char const*)
LDEvaluationSeriesContext_Method(LDServerSDKEvaluationSeriesContext eval_context) {
    LD_ASSERT(eval_context != nullptr);
    return AS_WRAPPER(eval_context)->context.Method().data();
}

LD_EXPORT(LDHookContext)
LDEvaluationSeriesContext_HookContext(
    LDServerSDKEvaluationSeriesContext eval_context) {
    LD_ASSERT(eval_context != nullptr);
    return AS_HOOK_CONTEXT(&AS_WRAPPER(eval_context)->context.HookCtx());
}

LD_EXPORT(char const*)
LDEvaluationSeriesContext_EnvironmentId(
    LDServerSDKEvaluationSeriesContext eval_context) {
    LD_ASSERT(eval_context != nullptr);
    auto env_id = AS_WRAPPER(eval_context)->context.EnvironmentId();
    return env_id.has_value() ? env_id->data() : nullptr;
}
