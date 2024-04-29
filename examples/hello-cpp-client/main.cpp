#include <launchdarkly/client_side/client.hpp>
#include <launchdarkly/context_builder.hpp>

#include <cstring>
#include <iostream>

// Set MOBILE_KEY to your LaunchDarkly mobile key.
#define MOBILE_KEY ""

// Set FEATURE_FLAG_KEY to the feature flag key you want to evaluate.
#define FEATURE_FLAG_KEY "my-boolean-flag"

// Set INIT_TIMEOUT_MILLISECONDS to the amount of time you will wait for
// the client to become initialized.
#define INIT_TIMEOUT_MILLISECONDS 3000

char const* get_with_env_fallback(char const* source_val,
                                  char const* env_variable,
                                  char const* error_msg);

using namespace launchdarkly;
using namespace launchdarkly::client_side;

int main() {
    char const* mobile_key = get_with_env_fallback(
        MOBILE_KEY, "LD_MOBILE_KEY",
        "Please edit main.c to set MOBILE_KEY to your LaunchDarkly mobile key "
        "first.\n\nAlternatively, set the LD_MOBILE_KEY environment "
        "variable.\n"
        "The value of MOBILE_KEY in main.c takes priority over LD_MOBILE_KEY.");

    auto config = ConfigBuilder(mobile_key).Build();
    if (!config) {
        std::cout << "error: config is invalid: " << config.error() << '\n';
        return 1;
    }

    auto context =
        ContextBuilder().Kind("user", "example-user-key").Name("Sandy").Build();

    Client client{std::move(*config), std::move(context)};

    client.Initialize();

    if (client.WaitForInitialization(
            std::chrono::milliseconds{INIT_TIMEOUT_MILLISECONDS})) {
        std::cout << "*** SDK successfully initialized!\n\n";
    } else {
        std::cout << "*** SDK failed to initialize\n";
        return 1;
    }

    bool const flag_value = client.BoolVariation(FEATURE_FLAG_KEY, false);

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

    if (char const* from_env = std::getenv(env_variable);
        from_env && strlen(from_env)) {
        return from_env;
    }

    std::cout << "*** " << error_msg << std::endl;
    std::exit(1);
}
