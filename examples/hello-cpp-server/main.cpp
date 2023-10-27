#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/server_side/client.hpp>

#include <cstring>
#include <iostream>

// Set SDK_KEY to your LaunchDarkly SDK key.
#define SDK_KEY ""

// Set FEATURE_FLAG_KEY to the feature flag key you want to evaluate.
#define FEATURE_FLAG_KEY "my-boolean-flag"

// Set INIT_TIMEOUT_MILLISECONDS to the amount of time you will wait for
// the client to become initialized.
#define INIT_TIMEOUT_MILLISECONDS 3000

char const* get_with_env_fallback(char const* source_val,
                                  char const* env_variable,
                                  char const* error_msg);
using namespace launchdarkly;
int main() {
    char const* sdk_key = get_with_env_fallback(
        SDK_KEY, "LD_SDK_KEY",
        "Please edit main.c to set SDK_KEY to your LaunchDarkly server-side "
        "SDK key "
        "first.\n\nAlternatively, set the LD_SDK_KEY environment "
        "variable.\n"
        "The value of SDK_KEY in main.c takes priority over LD_SDK_KEY.");

    auto config = server_side::ConfigBuilder(sdk_key).Build();
    if (!config) {
        std::cout << "error: config is invalid: " << config.error() << '\n';
        return 1;
    }

    auto client = server_side::Client(std::move(*config));

    auto start_result = client.StartAsync();
    auto status = start_result.wait_for(
        std::chrono::milliseconds(INIT_TIMEOUT_MILLISECONDS));
    if (status == std::future_status::ready) {
        if (start_result.get()) {
            std::cout << "*** SDK successfully initialized!\n\n";
        } else {
            std::cout << "*** SDK failed to initialize\n";
            return 1;
        }
    } else {
        std::cout << "*** SDK initialization didn't complete in "
                  << INIT_TIMEOUT_MILLISECONDS << "ms\n";
        return 1;
    }

    auto context =
        ContextBuilder().Kind("user", "example-user-key").Name("Sandy").Build();

    bool flag_value = client.BoolVariation(context, FEATURE_FLAG_KEY, false);

    std::cout << "*** Feature flag '" << FEATURE_FLAG_KEY << "' is "
              << (flag_value ? "true" : "false") << " for this user\n\n";

    return 0;
}

char const* get_with_env_fallback(char const* source_val,
                                  char const* env_variable,
                                  char const* error_msg) {
    if (strlen(source_val)) {
        return source_val;
    }

    char const* from_env = std::getenv(env_variable);
    if (from_env && strlen(from_env)) {
        return from_env;
    }

    std::cout << "*** " << error_msg << std::endl;
    std::exit(1);
}
