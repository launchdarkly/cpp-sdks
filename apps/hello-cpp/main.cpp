#include <launchdarkly/sse/client.hpp>
#include "launchdarkly/client_side/api.hpp"

#include <boost/asio/io_context.hpp>

#include "config/detail/builders/data_source_builder.hpp"
#include "console_backend.hpp"
#include "context_builder.hpp"
#include "launchdarkly/client_side/data_sources/detail/streaming_data_source.hpp"

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
    Logger logger(std::make_unique<ConsoleBackend>(LogLevel::kDebug, "Hello"));

    net::io_context ioc;

    char const* key = std::getenv("STG_SDK_KEY");
    if (!key) {
        std::cout << "Set environment variable STG_SDK_KEY to the sdk key";
        return 1;
    }

    Client client(
        ConfigBuilder(key)
            .DataSource(DataSourceBuilder()
                            .Method(DataSourceBuilder::Polling().PollInterval(
                                std::chrono::seconds{30}))
                            .WithReasons(true)
                            .UseReport(true))
            .Build()
            .value(),
        ContextBuilder().kind("user", "ryan").build());

    client.WaitForReadySync(std::chrono::seconds(30));

    auto value = client.BoolVariation("my-boolean-flag", false);
    LD_LOG(logger, LogLevel::kInfo) << "Value was: " << value;

    // Sit around.
    std::cout << "Press enter to exit" << std::endl;
    std::cin.get();
}
