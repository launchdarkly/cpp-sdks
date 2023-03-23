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

    const char* key = std::getenv("STG_SDK_KEY");
    if (!key){
        std::cout << "Set environment variable STG_SDK_KEY to the sdk key\n";
        return 1;
    }
    auto client = launchdarkly::sse::builder(ioc.get_executor(), "https://stream-stg.launchdarkly.com/all")
            .header("Authorization", key)
            .build();

    if (!client) {
        LD_LOG(logger, LogLevel::kError) << "Failed to build client";
        return 1;
    }

    client->run();
    ioc.run();
}
