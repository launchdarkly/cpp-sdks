// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include "c_bindings/config/client.h"

#include "c_error_converter.hpp"
#include "config/client.hpp"

using namespace launchdarkly::client_side;

#define TO_BUILDER(ptr) (reinterpret_cast<ConfigBuilder*>(ptr))
#define FROM_BUILDER(ptr) (reinterpret_cast<LDClientConfigBuilder>(ptr))
#define FROM_CONFIG(ptr) (reinterpret_cast<LDClientConfig>(ptr))

LD_EXPORT(LDClientConfigBuilder)
LDClientConfigBuilder_New(char const* sdk_key) {
    return FROM_BUILDER(new ConfigBuilder(sdk_key));
}

LD_EXPORT(LDError)
LDClientConfigBuilder_Build(LDClientConfigBuilder builder,
                            LDClientConfig* out_config) {
    return ConvertError<ConfigBuilder>(builder, out_config);
}
