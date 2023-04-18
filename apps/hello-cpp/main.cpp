#include <launchdarkly/sse/client.hpp>
#include "launchdarkly/client_side/api.hpp"

#include <boost/asio/io_context.hpp>

#include "console_backend.hpp"
#include "logger.hpp"

#include <iostream>
#include <utility>

namespace net = boost::asio;  // from <boost/asio.hpp>

using launchdarkly::ConsoleBackend;
using launchdarkly::Logger;
using launchdarkly::LogLevel;

int main() {
    Logger logger(std::make_unique<ConsoleBackend>("Hello"));

    net::io_context ioc;

    char const* key = std::getenv("STG_SDK_KEY");
    if (!key) {
        std::cout << "Set environment variable STG_SDK_KEY to the sdk key\n";
        return 1;
    }
    auto client =
        launchdarkly::sse::Builder(ioc.get_executor(),
                                   "https://stream-stg.launchdarkly.com/all")
            .header("Authorization", key)
            .receiver([&](launchdarkly::sse::Event ev) {
                LD_LOG(logger, LogLevel::kInfo) << "event: " << ev.type();
                LD_LOG(logger, LogLevel::kInfo)
                    << "data: " << std::move(ev).take();
            })
            .logger([&](std::string msg) {
                LD_LOG(logger, LogLevel::kDebug) << std::move(msg);
            })
            .build();

    if (!client) {
        LD_LOG(logger, LogLevel::kError) << "Failed to build client";
        return 1;
    }

    client->run();
    ioc.run();
}
