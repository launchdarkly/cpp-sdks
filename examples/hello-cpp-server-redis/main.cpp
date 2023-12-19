#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>

#include <launchdarkly/server_side/integrations/redis/redis_source.hpp>

#include <cstring>
#include <iostream>

// Set SDK_KEY to your LaunchDarkly SDK key.
#define SDK_KEY ""

// Set FEATURE_FLAG_KEY to the feature flag key you want to evaluate.
#define FEATURE_FLAG_KEY "my-boolean-flag"

// Set INIT_TIMEOUT_MILLISECONDS to the amount of time you will wait for
// the client to become initialized.
#define INIT_TIMEOUT_MILLISECONDS 3000

// Set REDIS_URI to your own Redis instance's URI.
#define REDIS_URI "redis://localhost:6379"

// Set REDIS_PREFIX to the prefix containing the launchdarkly
// environment data in Redis.
#define REDIS_PREFIX "launchdarkly"

char const* get_with_env_fallback(char const* source_val,
                                  char const* env_variable,
                                  char const* error_msg);
using namespace launchdarkly;
using namespace launchdarkly::server_side;

int main() {
    char const* sdk_key = get_with_env_fallback(
        SDK_KEY, "LD_SDK_KEY",
        "Please edit main.cpp to set SDK_KEY to your LaunchDarkly server-side "
        "SDK key "
        "first.\n\nAlternatively, set the LD_SDK_KEY environment "
        "variable.\n"
        "The value of SDK_KEY in main.c takes priority over LD_SDK_KEY.");

    auto config_builder = ConfigBuilder(sdk_key);

    using LazyLoad = server_side::config::builders::LazyLoadBuilder;

    auto redis = integrations::RedisDataSource::Create(REDIS_URI, REDIS_PREFIX);

    if (!redis) {
        std::cout << "error: redis config is invalid: " << redis.error()
                  << '\n';
        return 1;
    }

    config_builder.DataSystem().Method(
        LazyLoad()
            .Source(std::move(*redis))
            .CacheRefresh(std::chrono::seconds(30)));

    auto config = config_builder.Build();
    if (!config) {
        std::cout << "error: config is invalid: " << config.error() << '\n';
        return 1;
    }

    auto client = Client(std::move(*config));

    auto start_result = client.StartAsync();

    if (auto const status = start_result.wait_for(
            std::chrono::milliseconds(INIT_TIMEOUT_MILLISECONDS));
        status == std::future_status::ready) {
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

    auto const context =
        ContextBuilder().Kind("user", "example-user-key").Name("Sandy").Build();

    bool const flag_value =
        client.BoolVariation(context, FEATURE_FLAG_KEY, false);

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
