// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/bindings/c/config/builder.h>
#include <launchdarkly/config/client.hpp>
#include "../../../c_binding_helpers.hpp"

using namespace launchdarkly::client_side;

#define TO_BUILDER(ptr) (reinterpret_cast<ConfigBuilder*>(ptr))
#define FROM_BUILDER(ptr) (reinterpret_cast<LDClientConfigBuilder>(ptr))

LD_EXPORT(LDClientConfigBuilder)
LDClientConfigBuilder_New(char const* sdk_key) {
    ASSERT_NOT_NULL(sdk_key);
    return FROM_BUILDER(new ConfigBuilder(sdk_key));
}

LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_PollingBaseURL(LDClientConfigBuilder b,
                                                      char const* url) {
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(url);
    TO_BUILDER(b)->ServiceEndpoints().PollingBaseUrl(url);
}

LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_StreamingBaseURL(LDClientConfigBuilder b,
                                                        char const* url) {
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(url);
    TO_BUILDER(b)->ServiceEndpoints().StreamingBaseUrl(url);
}

LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_EventsBaseURL(LDClientConfigBuilder b,
                                                     char const* url) {
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(url);
    TO_BUILDER(b)->ServiceEndpoints().EventsBaseUrl(url);
}

LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_RelayProxy(LDClientConfigBuilder b,
                                                  char const* url) {
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(url);
    TO_BUILDER(b)->ServiceEndpoints().RelayProxy(url);
}

LD_EXPORT(LDStatus)
LDClientConfigBuilder_Build(LDClientConfigBuilder b,
                            LDClientConfig* out_config) {
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(out_config);
    return ConsumeBuilder<ConfigBuilder>(b, out_config);
}

LD_EXPORT(void)
LDClientConfigBuilder_Free(LDClientConfigBuilder builder) {
    if (ConfigBuilder* b = TO_BUILDER(builder)) {
        delete b;
    }
}

LD_EXPORT(void)
LDClientConfigBuilder_AppInfo_Identifier(LDClientConfigBuilder b,
                                         char const* app_id) {
    ASSERT_NOT_NULL(app_id);
    TO_BUILDER(b)->AppInfo().Identifier(app_id);
}

LD_EXPORT(void)
LDClientConfigBuilder_AppInfo_Version(LDClientConfigBuilder b,
                                      char const* app_version) {
    ASSERT_NOT_NULL(app_version);
    TO_BUILDER(b)->AppInfo().Version(app_version);
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
