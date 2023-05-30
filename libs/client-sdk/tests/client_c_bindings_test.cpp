#include <gtest/gtest.h>
#include <launchdarkly/bindings/c/config/builder.h>
#include <launchdarkly/bindings/c/context_builder.h>

#include <launchdarkly/client_side/bindings/c/sdk.h>

TEST(ClientBindings, MinimalInstantiation) {
    LDClientConfigBuilder cfg_builder = LDClientConfigBuilder_New("sdk-123");

    LDClientConfig config;
    LDStatus status = LDClientConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDContextBuilder ctx_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(ctx_builder, "user", "shadow");

    LDContext context = LDContextBuilder_Build(ctx_builder);

    LDClientSDK sdk = LDClientSDK_New(config, context);

    char const* version = LDClientSDK_Version(sdk);
    ASSERT_TRUE(version);
    ASSERT_STRNE(version, "");

    LDClientSDK_Free(sdk);
}
