#include <gtest/gtest.h>

#include <launchdarkly/client_side/bindings/c/config/builder.h>
#include <launchdarkly/client_side/bindings/c/sdk.h>

#include <launchdarkly/bindings/c/context_builder.h>

#include <launchdarkly/client_side/data_source_status.hpp>

#include <chrono>

using launchdarkly::client_side::data_sources::DataSourceStatus;

TEST(ClientBindings, MinimalInstantiation) {
    LDClientConfigBuilder cfg_builder = LDClientConfigBuilder_New("sdk-123");

    LDClientConfig config;
    LDStatus status = LDClientConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDContextBuilder ctx_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(ctx_builder, "user", "shadow");

    LDContext context = LDContextBuilder_Build(ctx_builder);

    LDClientSDK sdk = LDClientSDK_New(config, context);

    char const* version = LDClientSDK_Version();
    ASSERT_TRUE(version);
    ASSERT_STREQ(version, "3.2.1");  // {x-release-please-version}

    LDClientSDK_Free(sdk);
}

void FlagListenerFunction(char const* flag_key,
                          LDValue new_value,
                          LDValue old_value,
                          bool deleted,
                          void* user_data) {}

// This test registers a listener. It doesn't use the listener, but it
// will at least ensure 1.) Compilation, and 2.) Allow sanitizers to run.
TEST(ClientBindings, RegisterFlagListener) {
    LDClientConfigBuilder cfg_builder = LDClientConfigBuilder_New("sdk-123");
    LDClientConfigBuilder_Offline(cfg_builder, true);

    LDClientConfig config;
    LDStatus status = LDClientConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDContextBuilder ctx_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(ctx_builder, "user", "shadow");

    LDContext context = LDContextBuilder_Build(ctx_builder);

    LDClientSDK sdk = LDClientSDK_New(config, context);

    bool success = false;
    LDClientSDK_Start(sdk, 3000, &success);
    EXPECT_TRUE(success);

    struct LDFlagListener listener {
        reinterpret_cast<FlagChangedCallbackFn>(0x123),
            reinterpret_cast<void*>(0x456)
    };

    LDFlagListener_Init(&listener);
    ASSERT_EQ(listener.FlagChanged, nullptr);
    ASSERT_EQ(listener.UserData, nullptr);

    listener.UserData = const_cast<char*>("Potato");
    listener.FlagChanged = FlagListenerFunction;

    LDListenerConnection connection =
        LDClientSDK_FlagNotifier_OnFlagChange(sdk, "my-boolean-flag", listener);
    LDListenerConnection_Disconnect(connection);

    LDListenerConnection_Free(connection);
    LDClientSDK_Free(sdk);
}

void StatusListenerFunction(LDDataSourceStatus status, void* user_data) {
    EXPECT_EQ(LD_DATASOURCESTATUS_STATE_OFFLINE,
              LDDataSourceStatus_GetState(status));
}

// This test registers a listener. It doesn't use the listener, but it
// will at least ensure 1.) Compilation, and 2.) Allow sanitizers to run.
TEST(ClientBindings, RegisterDataSourceStatusChangeListener) {
    LDClientConfigBuilder cfg_builder = LDClientConfigBuilder_New("sdk-123");
    LDClientConfigBuilder_Offline(cfg_builder, true);

    LDClientConfig config;
    LDStatus status = LDClientConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDContextBuilder ctx_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(ctx_builder, "user", "shadow");

    LDContext context = LDContextBuilder_Build(ctx_builder);

    LDClientSDK sdk = LDClientSDK_New(config, context);

    struct LDDataSourceStatusListener listener {
        reinterpret_cast<DataSourceStatusCallbackFn>(0x123),
            reinterpret_cast<void*>(0x456)
    };
    LDDataSourceStatusListener_Init(&listener);

    ASSERT_EQ(listener.StatusChanged, nullptr);
    ASSERT_EQ(listener.UserData, nullptr);

    listener.UserData = const_cast<char*>("Potato");
    listener.StatusChanged = StatusListenerFunction;

    LDListenerConnection connection =
        LDClientSDK_DataSourceStatus_OnStatusChange(sdk, listener);

    bool success = false;
    LDClientSDK_Start(sdk, 3000, &success);
    EXPECT_TRUE(success);

    LDListenerConnection_Disconnect(connection);

    LDListenerConnection_Free(connection);
    LDClientSDK_Free(sdk);
}

TEST(ClientBindings, GetStatusOfOfflineClient) {
    LDClientConfigBuilder cfg_builder = LDClientConfigBuilder_New("sdk-123");
    LDClientConfigBuilder_Offline(cfg_builder, true);

    LDClientConfig config;
    LDStatus status = LDClientConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDContextBuilder ctx_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(ctx_builder, "user", "shadow");

    LDContext context = LDContextBuilder_Build(ctx_builder);

    LDClientSDK sdk = LDClientSDK_New(config, context);

    LDDataSourceStatus status_1 = LDClientSDK_DataSourceStatus_Status(sdk);
    EXPECT_EQ(LD_DATASOURCESTATUS_STATE_INITIALIZING,
              LDDataSourceStatus_GetState(status_1));

    bool success = false;
    LDClientSDK_Start(sdk, 3000, &success);

    LDDataSourceStatus status_2 = LDClientSDK_DataSourceStatus_Status(sdk);
    EXPECT_EQ(LD_DATASOURCESTATUS_STATE_OFFLINE,
              LDDataSourceStatus_GetState(status_2));

    EXPECT_EQ(nullptr, LDDataSourceStatus_GetLastError(status_2));

    EXPECT_NE(0, LDDataSourceStatus_StateSince(status_2));

    LDDataSourceStatus_Free(status_1);
    LDDataSourceStatus_Free(status_2);
    LDClientSDK_Free(sdk);
}

TEST(ClientBindings, ComplexDataSourceStatus) {
    DataSourceStatus status(
        DataSourceStatus::DataSourceState::kValid,
        std::chrono::time_point<std::chrono::system_clock>{
            std::chrono::seconds{200}},
        DataSourceStatus::ErrorInfo(
            DataSourceStatus::ErrorInfo::ErrorKind::kErrorResponse, 404,
            "Not found",
            std::chrono::time_point<std::chrono::system_clock>{
                std::chrono::seconds{100}}));

    EXPECT_EQ(LD_DATASOURCESTATUS_STATE_VALID,
              LDDataSourceStatus_GetState(
                  reinterpret_cast<LDDataSourceStatus>(&status)));

    EXPECT_EQ(200, LDDataSourceStatus_StateSince(
                       reinterpret_cast<LDDataSourceStatus>(&status)));

    LDDataSourceStatus_ErrorInfo info = LDDataSourceStatus_GetLastError(
        reinterpret_cast<LDDataSourceStatus>(&status));

    EXPECT_EQ(LD_DATASOURCESTATUS_ERRORKIND_ERROR_RESPONSE,
              LDDataSourceStatus_ErrorInfo_GetKind(info));

    EXPECT_EQ(std::string("Not found"),
              LDDataSourceStatus_ErrorInfo_Message(info));

    EXPECT_EQ(100, LDDataSourceStatus_ErrorInfo_Time(info));

    EXPECT_EQ(404, LDDataSourceStatus_ErrorInfo_StatusCode(info));

    LDDataSourceStatus_ErrorInfo_Free(info);
}
