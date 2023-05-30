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

    char const* version = LDClientSDK_Version();
    ASSERT_TRUE(version);
    ASSERT_STREQ(version, "0.1.0");  // {x-release-please-version}

    LDClientSDK_Free(sdk);
}

void ListenerFunction(char const* flag_key,
                      LDValue new_value,
                      LDValue old_value,
                      bool deleted,
                      void* user_data) {
}

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

    struct LDFlagListener listener{};
    LDFlagListener_Init(listener);
    listener.UserData =  const_cast<char*>("Potato");
    listener.FlagChanged = ListenerFunction;

    LDListenerConnection connection = LDClientSDK_FlagNotifier_OnFlagChange(sdk, "my-boolean-flag", listener);

    LDListenerConnection_Disconnect(connection);

    LDListenerConnection_Free(connection);
    LDClientSDK_Free(sdk);
}
