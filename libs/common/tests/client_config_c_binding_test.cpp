#include <gtest/gtest.h>
#include <launchdarkly/bindings/c/config/builder.h>

TEST(ClientConfigBindings, ConfigBuilderNewFree) {
    LDClientConfigBuilder builder = LDClientConfigBuilder_New("sdk-123");
    ASSERT_TRUE(builder);
    LDClientConfigBuilder_Free(builder);
}

TEST(ClientConfigBindings, ConfigBuilderEmptyResultsInError) {
    LDClientConfigBuilder builder = LDClientConfigBuilder_New(nullptr);

    LDClientConfig config = nullptr;
    LDStatus status = LDClientConfigBuilder_Build(builder, &config);

    ASSERT_FALSE(config);
    ASSERT_FALSE(LDStatus_Ok(status));
    ASSERT_TRUE(LDStatus_Error(status));

    LDStatus_Free(status);
    // LDClientConfigBuilder is consumed by Build; no need to free it.
}

TEST(ClientConfigBindings, MinimalValidConfig) {
    LDClientConfigBuilder builder = LDClientConfigBuilder_New("sdk-123");

    LDClientConfig config = nullptr;
    LDStatus status = LDClientConfigBuilder_Build(builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));
    ASSERT_TRUE(config);

    LDClientConfig_Free(config);
    // LDClientConfigBuilder is consumed by Build; no need to free it.
}
