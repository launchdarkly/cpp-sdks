#include <launchdarkly/bindings/c/context_builder.h>
#include <launchdarkly/bindings/c/memory_routines.h>
#include <launchdarkly/bindings/c/object_builder.h>
#include <launchdarkly/client_side/bindings/c/config/builder.h>
#include <launchdarkly/client_side/bindings/c/sdk.h>
#include <iostream>

LDClientConfigBuilder ConfigBuilder = nullptr;
LDClientSDK g_pLDClient = nullptr;

void setOffline() {
    if (ConfigBuilder) {
        LDClientConfigBuilder_Offline(ConfigBuilder, true);
    }
}

void setOnline() {
    if (ConfigBuilder) {
        LDClientConfigBuilder_Offline(ConfigBuilder, false);
    }
}

bool isOffline() {
    if (g_pLDClient) {
        LDDataSourceStatus status =
            LDClientSDK_DataSourceStatus_Status(g_pLDClient);
        LDDataSourceStatus_State statusState =
            LDDataSourceStatus_GetState(status);
        if (statusState ==
                LDDataSourceStatus_State::LD_DATASOURCESTATUS_STATE_OFFLINE ||
            statusState ==
                LDDataSourceStatus_State::LD_DATASOURCESTATUS_STATE_SHUTDOWN) {
            return true;
        }
    }

    return false;
}

int main() {
    char const* userid = "user-id";
    char const* mobile_key = "mob-key-abc-123";
    ConfigBuilder = LDClientConfigBuilder_New(mobile_key);
    LDClientConfigBuilder_Events_PrivateAttribute(ConfigBuilder, "hubId");

    LDClientConfig config;
    LDStatus status = LDClientConfigBuilder_Build(ConfigBuilder, &config);
    if (!LDStatus_Ok(status)) {
        std::cout << "ldcsdk client config builder failed";
    }

    LDContextBuilder context_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(context_builder, "user", userid);

    LDContextBuilder_Attributes_SetPrivate(context_builder, "user", "email",
                                           LDValue_NewString("abc@gmail.com"));

    LDContext context = LDContextBuilder_Build(context_builder);
    g_pLDClient = LDClientSDK_New(config, context);
    unsigned int maxwait = 10 * 1000; /* 10 seconds */

    bool initialized_successfully;
    if (LDClientSDK_Start(g_pLDClient, maxwait, &initialized_successfully)) {
        if (initialized_successfully) {
            std::cout << "LaunchDarkly: client Initialization Succeded."
                      << std::endl;
        } else {
            std::cout << "LaunchDarkly: client Initialization failed\n";
        }
    } else {
        std::cout << "LaunchDarkly: The client is still initializing.\n";
    }

    bool initialized = LDClientSDK_Initialized(g_pLDClient);

    setOffline();
    bool offline = isOffline();
}
