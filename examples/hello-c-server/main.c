#include <launchdarkly/server_side/bindings/c/config/builder.h>
#include <launchdarkly/server_side/bindings/c/sdk.h>

#include <launchdarkly/bindings/c/context_builder.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Set SDK_KEY to your LaunchDarkly SKD key.
#define SDK_KEY ""

// Set FEATURE_FLAG_KEY to the feature flag key you want to evaluate.
#define FEATURE_FLAG_KEY "my-boolean-flag"

// Set INIT_TIMEOUT_MILLISECONDS to the amount of time you will wait for
// the client to become initialized.
#define INIT_TIMEOUT_MILLISECONDS 3000

int main() {
    if (!strlen(SDK_KEY)) {
        printf(
            "*** Please edit main.c to set SDK_KEY to your LaunchDarkly "
            "SDK key first\n\n");
        return 1;
    }

    LDServerConfigBuilder config_builder = LDServerConfigBuilder_New(SDK_KEY);

    LDServerConfig config = NULL;
    LDStatus config_status =
        LDServerConfigBuilder_Build(config_builder, &config);
    if (!LDStatus_Ok(config_status)) {
        printf("error: config is invalid: %s", LDStatus_Error(config_status));
        return 1;
    }

    LDServerSDK client = LDServerSDK_New(config);

    bool initialized_successfully = false;
    if (LDServerSDK_Start(client, INIT_TIMEOUT_MILLISECONDS,
                          &initialized_successfully)) {
        if (initialized_successfully) {
            printf("*** SDK successfully initialized!\n\n");
        } else {
            printf("*** SDK failed to initialize\n");
            return 1;
        }
    } else {
        printf("SDK initialization didn't complete in %dms\n",
               INIT_TIMEOUT_MILLISECONDS);
        return 1;
    }

    LDContextBuilder context_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(context_builder, "user", "example-user-key");
    LDContextBuilder_Attributes_SetName(context_builder, "user", "Sandy");
    LDContext context = LDContextBuilder_Build(context_builder);

    bool flag_value =
        LDServerSDK_BoolVariation(client, context, FEATURE_FLAG_KEY, false);

    printf("*** Feature flag '%s' is %s for this user\n\n", FEATURE_FLAG_KEY,
           flag_value ? "true" : "false");

    // Here we ensure that the SDK shuts down cleanly and has a chance to
    // deliver analytics events to LaunchDarkly before the program exits. If
    // analytics events are not delivered, the user properties and flag usage
    // statistics will not appear on your dashboard. In a normal long-running
    // application, the SDK would continue running and events would be delivered
    // automatically in the background.

    LDContext_Free(context);
    LDServerSDK_Free(client);

    return 0;
}
