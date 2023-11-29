#include <gtest/gtest.h>

#include <launchdarkly/server_side/bindings/c/config/builder.h>
#include <launchdarkly/server_side/bindings/c/sdk.h>
#include <launchdarkly/server_side/data_source_status.hpp>

#include <launchdarkly/bindings/c/context_builder.h>

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
    ASSERT_STREQ(version, "0.1.0");  // {x-release-please-version}

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

    bool success = true;
    LDServerSDK_Start(sdk, 3000, &success);

    // Since we're offline, the SDK won't become initialized.
    EXPECT_FALSE(success);

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

    LDAllFlagsState_Free(state);
    LDContext_Free(context);
    LDServerSDK_Free(sdk);
}
