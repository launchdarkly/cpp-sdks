#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>

#include <cstring>
#include <iostream>

using namespace launchdarkly;
using namespace launchdarkly::server_side;

int main() {
    auto config = ConfigBuilder("sdk-key").Build();
    if (!config) {
        std::cout << "error: config is invalid: " << config.error() << '\n';
        return 1;
    }

    auto client = Client(std::move(*config));

    client.StartAsync();

    std::cout << client.Initialized() << '\n';

}
