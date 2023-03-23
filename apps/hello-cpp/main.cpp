#include <iostream>
#include <launchdarkly/api.hpp>
#include <launchdarkly/sse/sse.hpp>
#include <thread>
#include "console_backend.hpp"
#include "logger.hpp"

namespace net = boost::asio;  // from <boost/asio.hpp>

using launchdarkly::ConsoleBackend;
using launchdarkly::Logger;
using launchdarkly::LogLevel;

int main() {
    Logger logger(std::make_unique<ConsoleBackend>(LogLevel::kInfo, "Hello"));

    if (auto num = launchdarkly::foo()) {
        LD_LOG(logger, LogLevel::kInfo) << "Got: " << *num << '\n';
    } else {
        LD_LOG(logger, LogLevel::kInfo) << "Got nothing\n";
    }

    net::io_context ioc;

    // curl "https://stream-stg.launchdarkly.com/all?filter=even-flags-2" -H
    // "Authorization: sdk-66a5dbe0-8b26-445a-9313-761e7e3d381b" -v
    auto client =
        launchdarkly::sse::builder(ioc,
                                   "https://stream-stg.launchdarkly.com/all")
            .header("Authorization", "sdk-66a5dbe0-8b26-445a-9313-761e7e3d381b")
            .build();

    if (!client) {
        LD_LOG(logger, LogLevel::kError) << "Failed to build client";
        return 1;
    }

    std::thread t([&]() { ioc.run(); });

    client->run();
}
