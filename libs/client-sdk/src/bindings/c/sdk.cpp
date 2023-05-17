// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/bindings/c/array_builder.h>
#include <launchdarkly/bindings/c/value.h>
#include <launchdarkly/client_side/bindings/c/sdk.h>
#include <launchdarkly/c_binding_helpers.hpp>
#include <launchdarkly/client_side/client.hpp>
#include <launchdarkly/config/client.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_detail.hpp>
#include <launchdarkly/value.hpp>

using namespace launchdarkly::client_side;
using namespace launchdarkly;

struct Detail;

#define TO_SDK(ptr) (reinterpret_cast<Client*>(ptr))
#define FROM_SDK(ptr) (reinterpret_cast<LDClientSDK>(ptr))

#define FROM_DETAIL(ptr) (reinterpret_cast<LDEvalDetail>(ptr))

LD_EXPORT(LDClientSDK)
LDClientSDK_New(LDClientConfig config, LDContext context) {
    LD_ASSERT_NOT_NULL(config);
    LD_ASSERT_NOT_NULL(context);

    auto as_cfg = reinterpret_cast<Config*>(config);
    auto as_ctx = reinterpret_cast<Context*>(context);

    auto sdk = new Client(std::move(*as_cfg), std::move(*as_ctx));

    LDClientConfig_Free(config);
    LDContext_Free(context);

    return FROM_SDK(sdk);
}

LD_EXPORT(bool) LDClientSDK_Initialized(LDClientSDK sdk) {
    LD_ASSERT_NOT_NULL(sdk);

    return TO_SDK(sdk)->Initialized();
}

LD_EXPORT(void)
LDClientSDK_TrackEvent(LDClientSDK sdk, char const* event_name) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(event_name);

    TO_SDK(sdk)->Track(event_name);
}

LD_EXPORT(void)
LDClientSDK_TrackMetric(LDClientSDK sdk,
                        char const* event_name,
                        double metric_value,
                        LDValue value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(event_name);
    LD_ASSERT_NOT_NULL(value);

    auto as_value = reinterpret_cast<Value*>(value);

    TO_SDK(sdk)->Track(event_name, metric_value, std::move(*as_value));

    LDValue_Free(value);
}

LD_EXPORT(void)
LDClientSDK_TrackData(LDClientSDK sdk, char const* event_name, LDValue value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(event_name);
    LD_ASSERT_NOT_NULL(value);

    auto as_value = reinterpret_cast<Value*>(value);

    TO_SDK(sdk)->Track(event_name, std::move(*as_value));

    LDValue_Free(value);
}

LD_EXPORT(void)
LDClientSDK_Flush(LDClientSDK sdk, int milliseconds) {
    LD_ASSERT_NOT_NULL(sdk);

    TO_SDK(sdk)->FlushAsync();
}

LD_EXPORT(void)
LDClientSDK_Identify(LDClientSDK sdk, LDContext context, int milliseconds) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);

    auto as_ctx = reinterpret_cast<Context*>(context);
    auto future = TO_SDK(sdk)->IdentifyAsync(std::move(*as_ctx));

    LDContext_Free(context);

    if (milliseconds < 0) {
        return;
    }

    future.wait_for(std::chrono::milliseconds{milliseconds});
}

LD_EXPORT(bool)
LDClientSDK_BoolVariation(LDClientSDK sdk,
                          char const* flag_key,
                          bool default_value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(flag_key);

    return TO_SDK(sdk)->BoolVariation(flag_key, default_value);
}

LD_EXPORT(bool)
LDClientSDK_BoolVariationDetail(LDClientSDK sdk,
                                char const* flag_key,
                                bool default_value,
                                LDEvalDetail* out_detail) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(flag_key);

    auto internal_detail =
        TO_SDK(sdk)->BoolVariationDetail(flag_key, default_value);

    bool result = internal_detail.Value();

    if (!out_detail) {
        return result;
    }

    *out_detail = FROM_DETAIL(new CEvaluationDetail(internal_detail));

    return result;
}

LD_EXPORT(void) LDClientSDK_Free(LDClientSDK sdk) {
    delete TO_SDK(sdk);
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
