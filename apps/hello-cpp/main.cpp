#include <launchdarkly/client_side/client.hpp>

#include <boost/asio/io_context.hpp>

#include <launchdarkly/context_builder.hpp>

#include <iostream>

namespace net = boost::asio;  // from <boost/asio.hpp>

using launchdarkly::ContextBuilder;
using launchdarkly::LogLevel;
using launchdarkly::client_side::Client;
using launchdarkly::client_side::ConfigBuilder;
using launchdarkly::client_side::DataSourceBuilder;
using launchdarkly::config::shared::builders::LoggingBuilder;

int main() {
    net::io_context ioc;

    char const* key = std::getenv("STG_SDK_KEY");
    if (!key) {
        std::cout << "Set environment variable STG_SDK_KEY to the sdk key"
                  << std::endl;
        return 1;
    }

    auto config_builder = ConfigBuilder(key);

    config_builder.ServiceEndpoints()
        .PollingBaseUrl("http://sdk.launchdarkly.com")
        .StreamingBaseUrl("https://stream.launchdarkly.com")
        .EventsBaseUrl("https://events.launchdarkly.com");
    config_builder.DataSource()
        .Method(
            DataSourceBuilder::Polling().PollInterval(std::chrono::seconds{30}))
        .WithReasons(true)
        .UseReport(true);
    config_builder.Logging().Logging(
        LoggingBuilder::BasicLogging().Level(LogLevel::kDebug));
    config_builder.Events().FlushInterval(std::chrono::seconds(5));

    auto config = config_builder.Build();
    if (!config) {
        std::cout << config.error();
        return 1;
    }

    Client client(std::move(*config),
                  ContextBuilder().kind("user", "ryan").build());

    std::cout << "Initial Status: " << client.DataSourceStatus().Status()
              << std::endl;

    client.DataSourceStatus().OnDataSourceStatusChange([](auto status) {
        std::cout << "Got status: " << status << std::endl;
    });

    client.FlagNotifier().OnFlagChange("my-boolean-flag", [](auto event) {
        std::cout << "Got flag change: " << *event << std::endl;
    });

    client.WaitForReadySync(std::chrono::seconds(30));

    auto value = client.BoolVariationDetail("my-boolean-flag", false);
    std::cout << "Value was: " << *value << std::endl;
    if (auto reason = value.Reason()) {
        std::cout << "Reason was: " << *reason << std::endl;
    }

    // Sit around.
    std::cout << "Press enter to exit" << std::endl << std::endl;
    std::cin.get();
}
