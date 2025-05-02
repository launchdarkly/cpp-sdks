#include <gtest/gtest.h>

#include <launchdarkly/server_side/bindings/c/config/builder.h>
#include <launchdarkly/server_side/bindings/c/sdk.h>
#include <launchdarkly/server_side/config/config.hpp>
#include <launchdarkly/server_side/data_source_status.hpp>

#include <launchdarkly/bindings/c/context_builder.h>

#include <launchdarkly/error.hpp>

#include <boost/json/parse.hpp>

#include <chrono>

TEST(ClientBindings, MinimalInstantiation) {
    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDServerSDK sdk = LDServerSDK_New(config);

    char const* version = LDServerSDK_Version();
    ASSERT_TRUE(version);
    ASSERT_STREQ(version, "3.8.2");  // {x-release-please-version}

    LDServerSDK_Free(sdk);
}

void StatusListenerFunction(LDServerDataSourceStatus status, void* user_data) {
    EXPECT_EQ(LD_SERVERDATASOURCESTATUS_STATE_VALID,
              LDServerDataSourceStatus_GetState(status));
}

// This test registers a listener. It doesn't use the listener, but it
// will at least ensure 1.) Compilation, and 2.) Allow sanitizers to run.
TEST(ClientBindings, RegisterDataSourceStatusChangeListener) {
    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");
    LDServerConfigBuilder_Offline(cfg_builder, true);

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDServerSDK sdk = LDServerSDK_New(config);

    struct LDServerDataSourceStatusListener listener {};
    LDServerDataSourceStatusListener_Init(&listener);

    listener.UserData = const_cast<char*>("Potato");
    listener.StatusChanged = StatusListenerFunction;

    LDListenerConnection connection =
        LDServerSDK_DataSourceStatus_OnStatusChange(sdk, listener);

    bool success = false;
    LDServerSDK_Start(sdk, 3000, &success);
    EXPECT_TRUE(success);

    LDListenerConnection_Disconnect(connection);

    LDListenerConnection_Free(connection);
    LDServerSDK_Free(sdk);
}

TEST(ClientBindings, GetStatusOfOfflineClient) {
    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");
    LDServerConfigBuilder_Offline(cfg_builder, true);

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDServerSDK sdk = LDServerSDK_New(config);

    LDServerDataSourceStatus status_1 =
        LDServerSDK_DataSourceStatus_Status(sdk);
    EXPECT_EQ(LD_SERVERDATASOURCESTATUS_STATE_INITIALIZING,
              LDServerDataSourceStatus_GetState(status_1));

    bool success = false;
    LDServerSDK_Start(sdk, 3000, &success);

    LDServerDataSourceStatus status_2 =
        LDServerSDK_DataSourceStatus_Status(sdk);
    EXPECT_EQ(LD_SERVERDATASOURCESTATUS_STATE_VALID,
              LDServerDataSourceStatus_GetState(status_2));

    EXPECT_EQ(nullptr, LDServerDataSourceStatus_GetLastError(status_2));

    EXPECT_NE(0, LDServerDataSourceStatus_StateSince(status_2));

    LDServerDataSourceStatus_Free(status_1);
    LDServerDataSourceStatus_Free(status_2);
    LDServerSDK_Free(sdk);
}

TEST(ClientBindings, ComplexDataSourceStatus) {
    using namespace launchdarkly::server_side;

    DataSourceStatus status(
        DataSourceStatus::DataSourceState::kValid,
        std::chrono::time_point<std::chrono::system_clock>{
            std::chrono::seconds{200}},
        DataSourceStatus::ErrorInfo(
            DataSourceStatus::ErrorInfo::ErrorKind::kErrorResponse, 404,
            "Not found",
            std::chrono::time_point<std::chrono::system_clock>{
                std::chrono::seconds{100}}));

    EXPECT_EQ(LD_SERVERDATASOURCESTATUS_STATE_VALID,
              LDServerDataSourceStatus_GetState(
                  reinterpret_cast<LDServerDataSourceStatus>(&status)));

    EXPECT_EQ(200, LDServerDataSourceStatus_StateSince(
                       reinterpret_cast<LDServerDataSourceStatus>(&status)));

    LDDataSourceStatus_ErrorInfo info = LDServerDataSourceStatus_GetLastError(
        reinterpret_cast<LDServerDataSourceStatus>(&status));

    EXPECT_EQ(LD_DATASOURCESTATUS_ERRORKIND_ERROR_RESPONSE,
              LDDataSourceStatus_ErrorInfo_GetKind(info));

    EXPECT_EQ(std::string("Not found"),
              LDDataSourceStatus_ErrorInfo_Message(info));

    EXPECT_EQ(100, LDDataSourceStatus_ErrorInfo_Time(info));

    EXPECT_EQ(404, LDDataSourceStatus_ErrorInfo_StatusCode(info));

    LDDataSourceStatus_ErrorInfo_Free(info);
}

TEST(ClientBindings, AllFlagsState) {
    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDServerSDK sdk = LDServerSDK_New(config);

    LDContextBuilder ctx_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(ctx_builder, "user", "shadow");
    LDContext context = LDContextBuilder_Build(ctx_builder);

    LDAllFlagsState state =
        LDServerSDK_AllFlagsState(sdk, context, LD_ALLFLAGSSTATE_DEFAULT);

    ASSERT_FALSE(LDAllFlagsState_Valid(state));

    char* json = LDAllFlagsState_SerializeJSON(state);
    ASSERT_STREQ(json, "{\"$valid\":false,\"$flagsState\":{}}");
    LDMemory_FreeString(json);

    LDValue nonexistent_flag = LDAllFlagsState_Value(state, "nonexistent_flag");
    ASSERT_EQ(LDValue_Type(nonexistent_flag), LDValueType_Null);

    LDValue value_map = LDAllFlagsState_Map(state);
    ASSERT_EQ(LDValue_Type(value_map), LDValueType_Object);

    LDValue_ObjectIter value_iter = LDValue_ObjectIter_New(value_map);
    ASSERT_TRUE(value_iter);

    ASSERT_TRUE(LDValue_ObjectIter_End(value_iter));
    LDValue_ObjectIter_Free(value_iter);

    LDValue_Free(value_map);

    LDAllFlagsState_Free(state);
    LDContext_Free(context);
    LDServerSDK_Free(sdk);
}

TEST(ClientBindings, DoubleVariationPassesThroughDefault) {
    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDServerSDK sdk = LDServerSDK_New(config);

    LDContextBuilder ctx_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(ctx_builder, "user", "shadow");
    LDContext context = LDContextBuilder_Build(ctx_builder);

    std::string const flag = "weight";
    std::vector<double> values = {0.0,  0.0001, 0.5,  1.234,
                                  12.9, 13.211, 24.0, 1000.0};
    for (auto const& v : values) {
        ASSERT_EQ(LDServerSDK_DoubleVariation(sdk, context, "weight", v), v);
        ASSERT_EQ(LDServerSDK_DoubleVariationDetail(sdk, context, "weight", v,
                                                    LD_DISCARD_DETAIL),
                  v);
    }

    LDServerSDK_Free(sdk);
    LDContext_Free(context);
}

TEST(ClientBindings, CanSetEventConfigurationSuccessfully) {
    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");

    LDServerConfigBuilder_Events_Enabled(cfg_builder, false);
    LDServerConfigBuilder_Events_Capacity(cfg_builder, 100);
    LDServerConfigBuilder_Events_ContextKeysCapacity(cfg_builder, 100);
    LDServerConfigBuilder_Events_PrivateAttribute(cfg_builder, "email");
    LDServerConfigBuilder_Events_AllAttributesPrivate(cfg_builder, true);

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    launchdarkly::server_side::Config const* c =
        reinterpret_cast<launchdarkly::server_side::Config*>(config);

    ASSERT_EQ(c->Events().Capacity(), 100);
    ASSERT_EQ(c->Events().ContextKeysCacheCapacity(), 100);
    ASSERT_EQ(c->Events().PrivateAttributes(),
              launchdarkly::AttributeReference::SetType{"email"});
    ASSERT_TRUE(c->Events().AllAttributesPrivate());
    ASSERT_FALSE(c->Events().Enabled());

    LDServerConfig_Free(config);
}

TEST(ClientBindings, LazyLoadDataSource) {
    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");

    LDServerLazyLoadBuilder lazy_builder = LDServerLazyLoadBuilder_New();

    /* Omitting setting the source, which should cause a config error. */
    LDServerLazyLoadBuilder_CachePolicy(
        lazy_builder, LD_LAZYLOAD_CACHE_EVICTION_POLICY_DISABLED);
    LDServerLazyLoadBuilder_CacheRefreshMs(lazy_builder, 1000);

    LDServerConfigBuilder_DataSystem_LazyLoad(cfg_builder, lazy_builder);

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);

    ASSERT_FALSE(LDStatus_Ok(status));
    ASSERT_STREQ(
        LDStatus_Error(status),
        launchdarkly::ErrorToString(
            launchdarkly::Error::kConfig_DataSystem_LazyLoad_MissingSource));

    LDStatus_Free(status);
}

TEST(ClientBindings, TlsConfigurationSkipVerifyPeer) {
    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");

    LDServerHttpPropertiesTlsBuilder tls =
        LDServerHttpPropertiesTlsBuilder_New();
    LDServerHttpPropertiesTlsBuilder_SkipVerifyPeer(tls, true);

    LDServerConfigBuilder_HttpProperties_Tls(cfg_builder, tls);

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDServerConfig_Free(config);
}

TEST(ClientBindings, TlsConfigurationCustomCAFile) {
    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");

    LDServerHttpPropertiesTlsBuilder tls =
        LDServerHttpPropertiesTlsBuilder_New();
    LDServerHttpPropertiesTlsBuilder_CustomCAFile(tls, "/path/to/file.pem");

    LDServerConfigBuilder_HttpProperties_Tls(cfg_builder, tls);

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDServerConfig_Free(config);
}

TEST(ClientBindings, TlsConfigurationSystemCAFile) {
    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");

    LDServerHttpPropertiesTlsBuilder tls =
        LDServerHttpPropertiesTlsBuilder_New();

    // Set, then unset the file. This should not cause a configuration error.
    LDServerHttpPropertiesTlsBuilder_CustomCAFile(tls, "/path/to/file.pem");
    LDServerHttpPropertiesTlsBuilder_CustomCAFile(tls, "");

    LDServerConfigBuilder_HttpProperties_Tls(cfg_builder, tls);

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDServerConfig_Free(config);
}

TEST(ClientBindings, StreamingPayloadFilters) {
    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");

    LDServerDataSourceStreamBuilder stream_builder =
        LDServerDataSourceStreamBuilder_New();

    LDServerDataSourceStreamBuilder_Filter(stream_builder, "foo");

    LDServerConfigBuilder_DataSystem_BackgroundSync_Streaming(cfg_builder,
                                                              stream_builder);

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDServerConfig_Free(config);
}

TEST(ClientBindings, PollingPayloadFilters) {
    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");

    LDServerDataSourcePollBuilder poll_builder =
        LDServerDataSourcePollBuilder_New();

    LDServerDataSourcePollBuilder_Filter(poll_builder, "foo");

    LDServerConfigBuilder_DataSystem_BackgroundSync_Polling(cfg_builder,
                                                            poll_builder);

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDServerConfig_Free(config);
}
