#include <launchdarkly/bindings/c/logging/log_level.h>

#include <launchdarkly/client_side/client.hpp>
#include <launchdarkly/context_builder.hpp>

#include <cstring>
#include <iostream>
#include <fstream>
#include <filesystem>

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

/**
 * \brief Example persistence implementation. This implementation stores each
 * key as a file on disk in different directories based on their namespace.
 *
 * This is an example and is not resilient and therefore not intended for
 * production.
 */
class ExamplePersistence: public IPersistence {
   public:
    void Set(std::string storage_namespace,
             std::string key,
             std::string data) noexcept override {
        std::filesystem::create_directories("data/" + storage_namespace);
        std::ofstream file;
        file.open("data/" + storage_namespace + "/" + key);
        file << data;
        file.close();
    }

    void Remove(std::string storage_namespace,
                std::string key) noexcept override {
        std::filesystem::remove("data/" + storage_namespace + "/" + key);
    }

    std::optional<std::string> Read(std::string storage_namespace,
                                    std::string key) noexcept override {
        std::ifstream file;

        file.open("data/" + storage_namespace + "/" + key);
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            file.close();
            return buffer.str();
        }
        return std::nullopt;
    }
};

int main() {
    char const* mobile_key = get_with_env_fallback(
        MOBILE_KEY, "LD_MOBILE_KEY",
        "Please edit main.c to set MOBILE_KEY to your LaunchDarkly mobile key "
        "first.\n\nAlternatively, set the LD_MOBILE_KEY environment "
        "variable.\n"
        "The value of MOBILE_KEY in main.c takes priority over LD_MOBILE_KEY.");

    auto config_builder = ConfigBuilder(mobile_key);
    config_builder.Persistence().Custom(std::make_shared<ExamplePersistence>());
    auto config = config_builder.Build();
    if (!config) {
        std::cout << "error: config is invalid: " << config.error() << '\n';
        return 1;
    }

    auto context =
        ContextBuilder().Kind("user", "example-user-key").Name("Sandy").Build();

    auto client = Client(std::move(*config), std::move(context));

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
