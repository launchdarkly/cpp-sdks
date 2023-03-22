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

    const char* key = std::getenv("STG_SDK_KEY");
    if (!key){
        std::cout << "Set environment variable STG_SDK_KEY to the sdk key\n";
        return 1;
    }
    auto client = launchdarkly::sse::builder(ioc.get_executor(), "https://stream-stg.launchdarkly.com/all")
            .header("Authorization", key)
            .build();

    if (!client) {
        std::cout << "Failed to build client" << std::endl;
        return 1;
    }

    client->read();


//    client->on_event([](launchdarkly::sse::event_data e){
//        std::cout << "Got[" << e.get_type() << "] = <" << e.get_data() << ">\n";
//    });

    ioc.run();
}
