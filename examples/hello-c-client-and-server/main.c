/**
 * This application intends to verify that the symbols from the
 * client-side and server-side SDKs do not clash, thus enabling both SDKs to be
 * used within the same application.
 */

#include <launchdarkly/client_side/bindings/c/sdk.h>
#include <launchdarkly/server_side/bindings/c/sdk.h>

#include <launchdarkly/client_side/bindings/c/config/builder.h>
#include <launchdarkly/server_side/bindings/c/config/builder.h>

#include <launchdarkly/bindings/c/context_builder.h>

int main() {
    LDContextBuilder context_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(context_builder, "user", "example-user-key");
    LDContextBuilder_Attributes_SetName(context_builder, "user", "Sandy");
    LDContext context = LDContextBuilder_Build(context_builder);

    LDClientConfigBuilder client_config_builder =
        LDClientConfigBuilder_New("foo");
    LDClientConfig client_config;

    LDStatus client_config_status =
        LDClientConfigBuilder_Build(client_config_builder, &client_config);

    if (LDStatus_Ok(client_config_status)) {
        LDClientSDK client_sdk = LDClientSDK_New(client_config, context);
        LDClientSDK_Free(client_sdk);
    }

    LDServerConfigBuilder server_config_builder =
        LDServerConfigBuilder_New("foo");
    LDServerConfig server_config;

    LDStatus server_config_status =
        LDServerConfigBuilder_Build(server_config_builder, &server_config);

    if (LDStatus_Ok(server_config_status)) {
        LDServerSDK server_sdk = LDServerSDK_New(server_config);
        LDServerSDK_Free(server_sdk);
    }

    return 0;
}
