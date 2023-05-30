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

    struct LDFlagListener listener;
    LDFlagListener_Init(listener);
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

    struct LDDataSourceStatusListener listener;
    LDDataSourceStatusListener_Init(listener);

    listener.UserData = const_cast<char*>("Potato");
    listener.StatusChanged = StatusListenerFunction;

    LDListenerConnection connection =
        LDClientSDK_DataSourceStatus_OnStatusChange(sdk, listener);

    // TODO: Wait for ready.

    LDListenerConnection_Disconnect(connection);

    LDListenerConnection_Free(connection);
    LDClientSDK_Free(sdk);
}

TEST(ClientBindings, GetStatusOfOfflineClient) {
    LDClientConfigBuilder cfg_builder = LDClientConfigBuilder_New("sdk-123");
    LDClientConfigBuilder_Offline(cfg_builder, true);

    LDClientConfig config;
    LDClientConfigBuilder_Offline(cfg_builder, true);
    LDStatus status = LDClientConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDContextBuilder ctx_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(ctx_builder, "user", "shadow");

    LDContext context = LDContextBuilder_Build(ctx_builder);

    LDClientSDK sdk = LDClientSDK_New(config, context);

    LDDataSourceStatus ds_status = LDClientSDK_DataSourceStatus_Status(sdk);

    // TODO: Wait for ready.

    EXPECT_EQ(LD_DATASOURCESTATUS_STATE_OFFLINE,
              LDDataSourceStatus_GetState(ds_status));

    EXPECT_EQ(nullptr, LDDataSourceStatus_GetLastError(ds_status));

    EXPECT_NE(0, LDDataSourceStatus_StateSince(ds_status));

    LDDataSourceStatus_Free(ds_status);
}
