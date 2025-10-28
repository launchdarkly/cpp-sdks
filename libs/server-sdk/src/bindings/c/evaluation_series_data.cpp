#include <launchdarkly/server_side/bindings/c/evaluation_series_data.h>
#include <launchdarkly/server_side/hooks/hook.hpp>

#include <launchdarkly/bindings/c/value.h>
#include <launchdarkly/detail/c_binding_helpers.hpp>
#include <launchdarkly/value.hpp>
#include <memory>

#define AS_EVAL_SERIES_DATA(ptr) \
    (reinterpret_cast<launchdarkly::server_side::hooks::EvaluationSeriesData*>(ptr))

#define AS_EVAL_SERIES_DATA_CONST(ptr) \
    (reinterpret_cast<launchdarkly::server_side::hooks::EvaluationSeriesData const*>(ptr))

#define AS_EVAL_SERIES_DATA_BUILDER(ptr) \
    (reinterpret_cast<launchdarkly::server_side::hooks::EvaluationSeriesDataBuilder*>(ptr))

#define AS_VALUE(ptr) \
    (reinterpret_cast<LDValue>(const_cast<launchdarkly::Value*>(ptr)))

#define AS_CPP_VALUE(ptr) \
    (reinterpret_cast<launchdarkly::Value*>(ptr))

using launchdarkly::Value;
using launchdarkly::server_side::hooks::EvaluationSeriesData;
using launchdarkly::server_side::hooks::EvaluationSeriesDataBuilder;

LD_EXPORT(LDServerSDKEvaluationSeriesData)
LDEvaluationSeriesData_New(void) {
    return reinterpret_cast<LDServerSDKEvaluationSeriesData>(
        new EvaluationSeriesData());
}

LD_EXPORT(bool)
LDEvaluationSeriesData_GetValue(LDServerSDKEvaluationSeriesData data,
                                char const* key,
                                LDValue* out_value) {
    LD_ASSERT(data != nullptr);
    LD_ASSERT(key != nullptr);
    LD_ASSERT(out_value != nullptr);

    if (const auto result = AS_EVAL_SERIES_DATA_CONST(data)->Get(key); result.has_value()) {
        // Return a pointer to the existing value - no heap allocation needed
        *out_value = AS_VALUE(&result->get());
        return true;
    }
    *out_value = nullptr;
    return false;
}

LD_EXPORT(bool)
LDEvaluationSeriesData_GetPointer(LDServerSDKEvaluationSeriesData data,
                                  char const* key,
                                  void** out_pointer) {
    LD_ASSERT(data != nullptr);
    LD_ASSERT(key != nullptr);
    LD_ASSERT(out_pointer != nullptr);

    const auto result = AS_EVAL_SERIES_DATA_CONST(data)->GetShared(key);
    if (result.has_value()) {
        // Extract the raw pointer from shared_ptr<any>
        if (*result != nullptr) {
            try {
                *out_pointer = std::any_cast<void*>(*result->get());
                return true;
            } catch (std::bad_any_cast const&) {
                // The stored value wasn't a void*, return false
            }
        }
    }
    *out_pointer = nullptr;
    return false;
}

LD_EXPORT(LDServerSDKEvaluationSeriesDataBuilder)
LDEvaluationSeriesData_NewBuilder(LDServerSDKEvaluationSeriesData data) {
    if (data) {
        return reinterpret_cast<LDServerSDKEvaluationSeriesDataBuilder>(
            new EvaluationSeriesDataBuilder(*AS_EVAL_SERIES_DATA_CONST(data)));
    }
    return reinterpret_cast<LDServerSDKEvaluationSeriesDataBuilder>(
        new EvaluationSeriesDataBuilder());
}

LD_EXPORT(void)
LDEvaluationSeriesData_Free(LDServerSDKEvaluationSeriesData data) {
    delete AS_EVAL_SERIES_DATA(data);
}

LD_EXPORT(void)
LDEvaluationSeriesDataBuilder_SetValue(
    LDServerSDKEvaluationSeriesDataBuilder builder,
    char const* key,
    LDValue value) {
    LD_ASSERT(builder != nullptr);
    LD_ASSERT(key != nullptr);
    LD_ASSERT(value != nullptr);

    // Copy the value into the builder
    AS_EVAL_SERIES_DATA_BUILDER(builder)->Set(key, *AS_CPP_VALUE(value));
}

LD_EXPORT(void)
LDEvaluationSeriesDataBuilder_SetPointer(
    LDServerSDKEvaluationSeriesDataBuilder builder,
    char const* key,
    void* pointer) {
    LD_ASSERT(builder != nullptr);
    LD_ASSERT(key != nullptr);

    const auto shared_any = std::make_shared<std::any>(
        pointer);
    // The "any" wrapper will be allocated and deleted, but the contents
    // of the "any" will not.
    AS_EVAL_SERIES_DATA_BUILDER(builder)->SetShared(key, shared_any);
}

LD_EXPORT(LDServerSDKEvaluationSeriesData)
LDEvaluationSeriesDataBuilder_Build(
    LDServerSDKEvaluationSeriesDataBuilder builder) {
    LD_ASSERT(builder != nullptr);

    auto* cpp_builder = AS_EVAL_SERIES_DATA_BUILDER(builder);
    auto* result = new EvaluationSeriesData(std::move(*cpp_builder).Build());
    delete cpp_builder;
    return reinterpret_cast<LDServerSDKEvaluationSeriesData>(result);
}

LD_EXPORT(void)
LDEvaluationSeriesDataBuilder_Free(
    LDServerSDKEvaluationSeriesDataBuilder builder) {
    delete AS_EVAL_SERIES_DATA_BUILDER(builder);
}
