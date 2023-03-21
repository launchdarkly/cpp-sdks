#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <launchdarkly/api.hpp>
#include <launchdarkly/sse/sse.hpp>
#include <thread>

namespace net = boost::asio;  // from <boost/asio.hpp>

int main() {
    if (auto num = launchdarkly::foo()) {
        std::cout << "Got: " << *num << '\n';
    } else {
        std::cout << "Got nothing\n";
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
        std::cout << "Failed to build client" << std::endl;
        return 1;
    }

    std::thread t([&]() { ioc.run(); });

    client->run();
}
