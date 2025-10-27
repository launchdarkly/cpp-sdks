#include <launchdarkly/server_side/bindings/c/track_series_context.h>
#include "context_wrappers.hpp"

#include <launchdarkly/bindings/c/context.h>
#include <launchdarkly/bindings/c/value.h>
#include <launchdarkly/detail/c_binding_helpers.hpp>

#define AS_WRAPPER(ptr) \
    (reinterpret_cast<launchdarkly::server_side::bindings::TrackSeriesContextWrapper const*>(ptr))

#define AS_CONTEXT(ptr) \
    (reinterpret_cast<LDContext>(const_cast<launchdarkly::Context*>(ptr)))

#define AS_VALUE(ptr) \
    (reinterpret_cast<LDValue>(const_cast<launchdarkly::Value*>(ptr)))

#define AS_HOOK_CONTEXT(ptr) \
    (reinterpret_cast<LDHookContext>(const_cast<launchdarkly::server_side::hooks::HookContext*>(ptr)))

LD_EXPORT(char const*)
LDTrackSeriesContext_Key(LDServerSDKTrackSeriesContext track_context) {
    LD_ASSERT(track_context != nullptr);
    return AS_WRAPPER(track_context)->context.Key().data();
}

LD_EXPORT(LDContext)
LDTrackSeriesContext_Context(LDServerSDKTrackSeriesContext track_context) {
    LD_ASSERT(track_context != nullptr);
    return AS_CONTEXT(&AS_WRAPPER(track_context)->context.TrackContext());
}

LD_EXPORT(bool)
LDTrackSeriesContext_Data(LDServerSDKTrackSeriesContext track_context,
                          LDValue* out_data) {
    LD_ASSERT(track_context != nullptr);
    LD_ASSERT(out_data != nullptr);

    auto data = AS_WRAPPER(track_context)->context.Data();
    if (data.has_value()) {
        // Return a pointer to the existing value - no heap allocation needed
        *out_data = AS_VALUE(&data->get());
        return true;
    }
    *out_data = nullptr;
    return false;
}

LD_EXPORT(bool)
LDTrackSeriesContext_MetricValue(LDServerSDKTrackSeriesContext track_context,
                                  double* out_metric_value) {
    LD_ASSERT(track_context != nullptr);
    LD_ASSERT(out_metric_value != nullptr);

    auto metric = AS_WRAPPER(track_context)->context.MetricValue();
    if (metric.has_value()) {
        *out_metric_value = *metric;
        return true;
    }
    return false;
}

LD_EXPORT(LDHookContext)
LDTrackSeriesContext_HookContext(LDServerSDKTrackSeriesContext track_context) {
    LD_ASSERT(track_context != nullptr);
    return AS_HOOK_CONTEXT(&AS_WRAPPER(track_context)->context.HookCtx());
}

LD_EXPORT(char const*)
LDTrackSeriesContext_EnvironmentId(LDServerSDKTrackSeriesContext track_context) {
    LD_ASSERT(track_context != nullptr);
    auto env_id = AS_WRAPPER(track_context)->context.EnvironmentId();
    return env_id.has_value() ? env_id->data() : nullptr;
}
