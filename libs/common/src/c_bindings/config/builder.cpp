// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include "launchdarkly/c_bindings/config/builder.h"

#include "launchdarkly/config/client.hpp"
#include "launchdarkly/c_binding_helpers.hpp"

using namespace launchdarkly::client_side;

#define TO_BUILDER(ptr) (reinterpret_cast<ConfigBuilder*>(ptr))
#define FROM_BUILDER(ptr) (reinterpret_cast<LDClientConfigBuilder>(ptr))

LD_EXPORT(LDClientConfigBuilder)
LDClientConfigBuilder_New(char const* sdk_key) {
    return FROM_BUILDER(new ConfigBuilder(sdk_key ? sdk_key : ""));
}

LD_EXPORT(void)
LDClientConfigBuilder_Free(LDClientConfigBuilder builder) {
    if (ConfigBuilder* b = TO_BUILDER(builder)) {
        delete b;
    }
}

LD_EXPORT(LDStatus)
LDClientConfigBuilder_Build(LDClientConfigBuilder builder,
                            LDClientConfig* out_config) {
    return ConsumeBuilder<ConfigBuilder>(builder, out_config);
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
