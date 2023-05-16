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

TEST(ClientConfigBindings, AllConfigs) {
    LDClientConfigBuilder builder = LDClientConfigBuilder_New("sdk-123");

    LDClientConfigBuilder_Offline(builder, false);

    LDClientConfigBuilder_AppInfo_Identifier(builder, "app");
    LDClientConfigBuilder_AppInfo_Version(builder, "v1.2.3");

    LDClientConfigBuilder_Events_Enabled(builder, true);
    LDClientConfigBuilder_Events_Capacity(builder, 100);
    LDClientConfigBuilder_Events_FlushIntervalMs(builder, 1000);
    LDClientConfigBuilder_Events_AllAttributesPrivate(builder, false);
    LDClientConfigBuilder_Events_PrivateAttribute(builder, "/foo/bar");

    LDClientConfigBuilder_DataSource_UseReport(builder, true);
    LDClientConfigBuilder_DataSource_WithReasons(builder, true);

    LDDataSourceStreamBuilder stream_builder = LDDataSourceStreamBuilder_New();
    LDDataSourceStreamBuilder_InitialReconnectDelayMs(stream_builder, 500);
    LDClientConfigBuilder_DataSource_MethodStream(builder, stream_builder);

    LDDataSourcePollBuilder poll_builder = LDDataSourcePollBuilder_New();
    LDDataSourcePollBuilder_IntervalS(poll_builder, 10);
    LDClientConfigBuilder_DataSource_MethodPoll(builder, poll_builder);

    LDClientConfigBuilder_HttpProperties_Header(builder, "foo", "bar");
    LDClientConfigBuilder_HttpProperties_WrapperName(builder, "wrapper");
    LDClientConfigBuilder_HttpProperties_WrapperVersion(builder, "v1.2.3");

    LDClientConfig config = nullptr;
    LDStatus status = LDClientConfigBuilder_Build(builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));
    ASSERT_TRUE(config);

    LDClientConfig_Free(config);
}
