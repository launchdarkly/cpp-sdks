#include <launchdarkly/client_side/client.hpp>
#include <launchdarkly/context_builder.hpp>

#include <iostream>

// Set MOBILE_KEY to your LaunchDarkly mobile key.
#define MOBILE_KEY ""

// Set FEATURE_FLAG_KEY to the feature flag key you want to evaluate.
#define FEATURE_FLAG_KEY "my-boolean-flag"

using namespace launchdarkly;
int main() {
    if (!strlen(MOBILE_KEY)) {
        printf(
            "*** Please edit main.c to set MOBILE_KEY to your LaunchDarkly "
            "mobile key first\n\n");
        return 1;
    }

    auto config = client_side::ConfigBuilder(MOBILE_KEY).Build();
    if (!config) {
        std::cout << "error: config is invalid: " << config.error() << '\n';
        return 1;
    }

    auto context =
        ContextBuilder().kind("user", "example-user-key").name("Sandy").build();

    auto client = client_side::Client(std::move(*config), std::move(context));

    if (client.Initialized()) {
        std::cout << "*** SDK successfully initialized!\n\n";
    } else {
        std::cout << "*** SDK failed to initialize\n\n";
        return 1;
    }

    bool flag_value = client.BoolVariation(FEATURE_FLAG_KEY, false);

    std::cout << "*** Feature flag '" << FEATURE_FLAG_KEY << "' is "
              << (flag_value ? "true" : "false") << " for this user\n\n";

    return 0;
}
