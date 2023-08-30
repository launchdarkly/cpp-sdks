#include <gtest/gtest.h>

#include <launchdarkly/server_side/bindings/c/config/builder.h>
#include <launchdarkly/server_side/bindings/c/sdk.h>
#include <launchdarkly/server_side/data_source_status.hpp>

#include <launchdarkly/bindings/c/context_builder.h>

#include <boost/json/parse.hpp>

#include <chrono>

using launchdarkly::server_side::data_sources::DataSourceStatus;

TEST(ClientBindings, MinimalInstantiation) {
    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDServerSDK sdk = LDServerSDK_New(config);

    char const* version = LDServerSDK_Version();
    ASSERT_TRUE(version);
    ASSERT_STREQ(version, "0.1.0");  // {x-release-please-version}

    LDServerSDK_Free(sdk);
}

void FlagListenerFunction(char const* flag_key,
                          LDValue new_value,
                          LDValue old_value,
                          bool deleted,
                          void* user_data) {}

// This test registers a listener. It doesn't use the listener, but it
// will at least ensure 1.) Compilation, and 2.) Allow sanitizers to run.
TEST(ClientBindings, RegisterFlagListener) {
    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");
    LDServerConfigBuilder_Offline(cfg_builder, true);

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDServerSDK sdk = LDServerSDK_New(config);

    bool success = false;
    LDServerSDK_Start(sdk, 3000, &success);
    EXPECT_TRUE(success);

    struct LDFlagListener listener {};
    LDFlagListener_Init(&listener);
    listener.UserData = const_cast<char*>("Potato");
    listener.FlagChanged = FlagListenerFunction;

    LDListenerConnection connection =
        LDServerSDK_FlagNotifier_OnFlagChange(sdk, "my-boolean-flag", listener);
    LDListenerConnection_Disconnect(connection);

    LDListenerConnection_Free(connection);
    LDServerSDK_Free(sdk);
}

void StatusListenerFunction(LDDataSourceStatus status, void* user_data) {
    EXPECT_EQ(LD_DATASOURCESTATUS_STATE_OFFLINE,
              LDDataSourceStatus_GetState(status));
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

    struct LDDataSourceStatusListener listener {};
    LDDataSourceStatusListener_Init(listener);

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

    LDDataSourceStatus status_1 = LDServerSDK_DataSourceStatus_Status(sdk);
    EXPECT_EQ(LD_DATASOURCESTATUS_STATE_INITIALIZING,
              LDDataSourceStatus_GetState(status_1));

    bool success = false;
    LDServerSDK_Start(sdk, 3000, &success);

    LDDataSourceStatus status_2 = LDServerSDK_DataSourceStatus_Status(sdk);
    EXPECT_EQ(LD_DATASOURCESTATUS_STATE_OFFLINE,
              LDDataSourceStatus_GetState(status_2));

    EXPECT_EQ(nullptr, LDDataSourceStatus_GetLastError(status_2));

    EXPECT_NE(0, LDDataSourceStatus_StateSince(status_2));

    LDDataSourceStatus_Free(status_1);
    LDDataSourceStatus_Free(status_2);
    LDServerSDK_Free(sdk);
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

    LDAllFlagsState_Free(state);
    LDContext_Free(context);
    LDServerSDK_Free(sdk);
}
