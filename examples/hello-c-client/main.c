#include <launchdarkly/client_side/bindings/c/sdk.h>

#include <launchdarkly/bindings/c/context_builder.h>
#include <launchdarkly/client_side/bindings/c/config/builder.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Set MOBILE_KEY to your LaunchDarkly mobile key.
#define MOBILE_KEY ""

// Set FEATURE_FLAG_KEY to the feature flag key you want to evaluate.
#define FEATURE_FLAG_KEY "my-boolean-flag"

// Set INIT_TIMEOUT_MILLISECONDS to the amount of time you will wait for
// the client to become initialized.
#define INIT_TIMEOUT_MILLISECONDS 3000

char const* get_with_env_fallback(char const* source_val,
                                  char const* env_variable,
                                  char const* error);

#if LD_DEBUG_LOGGING
#define print(...) printf(__VA_ARGS__)
#else
#define print(...) \
    do {           \
    } while (0)
#endif

int main() {
    char const* mobile_key = get_with_env_fallback(
        MOBILE_KEY, "LD_MOBILE_KEY",
        "Please edit main.c to set MOBILE_KEY to your LaunchDarkly mobile key "
        "first.\n\nAlternatively, set the LD_MOBILE_KEY environment "
        "variable.\n"
        "The value of MOBILE_KEY in main.c takes priority over LD_MOBILE_KEY.");

#if LD_DEBUG_LOGGING
    printf("Debug logging enabled\n");
#endif

    print("using mobile key: %s\n", mobile_key);
    LDClientConfigBuilder config_builder =
        LDClientConfigBuilder_New(mobile_key);

    print("created config builder\n");
    LDClientConfig config = NULL;

    LDStatus config_status =
        LDClientConfigBuilder_Build(config_builder, &config);

    print("built config\n");

    if (!LDStatus_Ok(config_status)) {
        printf("error: config is invalid: %s", LDStatus_Error(config_status));
        return 1;
    }

    print("creating context builder\n");

    LDContextBuilder context_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(context_builder, "user", "example-user-key");
    LDContextBuilder_Attributes_SetName(context_builder, "user", "Sandy");
    LDContext context = LDContextBuilder_Build(context_builder);

    print("created context, now creating SDK\n");
    LDClientSDK client = LDClientSDK_New(config, context);

    print("created SDK\n");
    bool initialized_successfully = false;
    if (LDClientSDK_Start(client, INIT_TIMEOUT_MILLISECONDS,
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

    print("evaluating flag: %s\n", FEATURE_FLAG_KEY);
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

char const* get_with_env_fallback(char const* source_val,
                                  char const* env_variable,
                                  char const* error_msg) {
    if (strlen(source_val)) {
        return source_val;
    }

    char const* from_env = getenv(env_variable);
    if (from_env && strlen(from_env)) {
        return from_env;
    }

    printf("*** %s\n\n", error_msg);
    exit(1);
}
