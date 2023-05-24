#include <launchdarkly/client_side/bindings/c/sdk.h>

#include <launchdarkly/bindings/c/config/builder.h>
#include <launchdarkly/bindings/c/context_builder.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Set MOBILE_KEY to your LaunchDarkly mobile key.
#define MOBILE_KEY ""

// Set FEATURE_FLAG_KEY to the feature flag key you want to evaluate.
#define FEATURE_FLAG_KEY "my-boolean-flag"

int main() {
    if (!strlen(MOBILE_KEY)) {
        printf(
            "*** Please edit main.c to set MOBILE_KEY to your LaunchDarkly "
            "mobile key first\n\n");
        return 1;
    }

    LDClientConfigBuilder config_builder =
        LDClientConfigBuilder_New(MOBILE_KEY);

    LDClientConfig config;
    LDStatus config_status =
        LDClientConfigBuilder_Build(config_builder, &config);
    if (!LDStatus_Ok(config_status)) {
        printf("error: config is invalid: %s", LDStatus_Error(config_status));
        return 1;
    }

    LDContextBuilder context_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(context_builder, "user", "example-user-key");
    LDContextBuilder_Attributes_SetName(context_builder, "user", "Sandy");
    LDContext context = LDContextBuilder_Build(context_builder);

    LDClientSDK client = LDClientSDK_New(config, context);

    bool initialized_successfully;
    if (LDClientSDK_Start(client, 5000, &initialized_successfully)) {
        if (initialized_successfully) {
            printf("*** SDK successfully initialized!\n\n");
        } else {
            printf("*** SDK failed to initialize\n");
            return 1;
        }
    } else {
        printf("SDK didn't initialize in time\n");
        return 1;
    }

    bool flag_value =
        LDClientSDK_BoolVariation(client, FEATURE_FLAG_KEY, false);

    printf("*** Feature flag '%s' is %s for this user\n\n", FEATURE_FLAG_KEY,
           flag_value ? "true" : "false");

    // Here we ensure that the SDK shuts down cleanly and has a chance to
    // deliver analytics events to LaunchDarkly before the program exits. If
    // analytics events are not delivered, the user properties and flag usage
    // statistics will not appear on your dashboard. In a normal long-running
    // application, the SDK would continue running and events would be delivered
    // automatically in the background.

    LDClientSDK_Free(client);

    return 0;
}
