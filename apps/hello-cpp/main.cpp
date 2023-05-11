#include <launchdarkly/sse/client.hpp>
#include <launchdarkly/client_side/api.hpp>

#include <boost/asio/io_context.hpp>

#include <launchdarkly/client_side/data_sources/detail/streaming_data_source.hpp>
#include <launchdarkly/console_backend.hpp>
#include <launchdarkly/context_builder.hpp>

#include <iostream>

namespace net = boost::asio;  // from <boost/asio.hpp>

using launchdarkly::ConsoleBackend;
using launchdarkly::ContextBuilder;
using launchdarkly::Logger;
using launchdarkly::LogLevel;
using launchdarkly::client_side::Client;
using launchdarkly::client_side::ConfigBuilder;
using launchdarkly::client_side::DataSourceBuilder;
using launchdarkly::client_side::flag_manager::detail::FlagManager;
using launchdarkly::client_side::flag_manager::detail::FlagUpdater;

int main() {
    Logger logger(std::make_unique<ConsoleBackend>("Hello"));

    net::io_context ioc;

    char const* key = std::getenv("STG_SDK_KEY");
    if (!key) {
        std::cout << "Set environment variable STG_SDK_KEY to the sdk key";
        return 1;
    }

    Client client(
        ConfigBuilder(key)
            .ServiceEndpoints(
                launchdarkly::client_side::EndpointsBuilder()
                    // Set to http to demonstrate redirect to https.
                    .PollingBaseUrl("http://sdk.launchdarkly.com")
                    .StreamingBaseUrl("https://stream.launchdarkly.com")
                    .EventsBaseUrl("https://events.launchdarkly.com"))
            .DataSource(DataSourceBuilder()
                            .Method(DataSourceBuilder::Polling().PollInterval(
                                std::chrono::seconds{30}))
                            .WithReasons(true)
                            .UseReport(true))
            .Events(launchdarkly::client_side::EventsBuilder().FlushInterval(
                std::chrono::seconds(5)))
            .Build()
            .value(),
        ContextBuilder().kind("user", "ryan").build());

    LD_LOG(logger, LogLevel::kInfo)
        << "Initial Status: " << client.DataSourceStatus().Status();

    client.DataSourceStatus().OnDataSourceStatusChange([&logger](auto status) {
        LD_LOG(logger, LogLevel::kInfo) << "Got status: " << status;
    });

    client.FlagNotifier().OnFlagChange(
        "my-boolean-flag", [&logger](auto event) {
            LD_LOG(logger, LogLevel::kInfo) << "Got flag change: " << *event;
        });

    client.WaitForReadySync(std::chrono::seconds(30));

    auto value = client.BoolVariationDetail("my-boolean-flag", false);
    LD_LOG(logger, LogLevel::kInfo) << "Value was: " << *value;
    if (auto reason = value.Reason()) {
        LD_LOG(logger, LogLevel::kInfo) << "Reason was: " << *reason;
    }

    // Sit around.
    std::cout << "Press enter to exit" << std::endl;
    std::cin.get();
}
