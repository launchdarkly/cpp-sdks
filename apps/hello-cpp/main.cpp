#include <launchdarkly/sse/client.hpp>
#include "launchdarkly/client_side/api.hpp"

#include <boost/asio/io_context.hpp>

#include "console_backend.hpp"
#include "context_builder.hpp"
#include "launchdarkly/client_side/api.hpp"
#include "launchdarkly/client_side/data_sources/detail/streaming_data_source.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_manager.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_updater.hpp"
#include "logger.hpp"

#include <iostream>
#include <utility>

namespace net = boost::asio;  // from <boost/asio.hpp>

using launchdarkly::ConsoleBackend;
using launchdarkly::ContextBuilder;
using launchdarkly::Logger;
using launchdarkly::LogLevel;
using launchdarkly::client::ConfigBuilder;
using launchdarkly::client_side::Client;
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

            Client client(ConfigBuilder(key).build().value(),
                          ContextBuilder().kind("user", "ryan").build());

            std::thread doing_stuff([&]() {
                while (true) {
                    auto value = client.BoolVariation("my-boolean-flag", false);
                    LD_LOG(logger, LogLevel::kInfo) << "Value was: " << value;
                    sleep(10);
                }
            });

    while (true) {    }
}
