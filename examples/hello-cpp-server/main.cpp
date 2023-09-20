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

using namespace launchdarkly;
int main() {
    if (!strlen(SDK_KEY)) {
        printf(
            "*** Please edit main.cpp to set SDK_KEY to your LaunchDarkly "
            "SDK key first\n\n");
        return 1;
    }

    auto cfg_builder = server_side::ConfigBuilder(SDK_KEY);
    cfg_builder.DataSources()
        .Source()
        .Method(
            server_side::DataSourceBuilder::Streaming().InitialReconnectDelay(
                std::chrono::seconds(1)))
        .Bootstrap(true)
        .Order(3);

    cfg_builder.DataSources()
        .Bootstrap()
        .Order(launchdarkly::config::shared::builders::BootstrapBuilder::Order::
                   Random)
        .RandomSeed(1234);

    auto config = cfg_builder.Build();
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
