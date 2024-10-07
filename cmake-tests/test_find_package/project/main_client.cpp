#include <launchdarkly/client_side/client.hpp>
#include <launchdarkly/context_builder.hpp>

#include <cstring>
#include <iostream>

using namespace launchdarkly;
using namespace launchdarkly::client_side;

int main() {
    auto config = ConfigBuilder("sdk-key").Build();
    if (!config) {
        std::cout << "error: config is invalid: " << config.error() << '\n';
        return 1;
    }

    auto context =
      ContextBuilder().Kind("user", "example-user-key").Name("Sandy").Build();

    auto client = Client(std::move(*config), std::move(context));

    client.StartAsync();

    std::cout << client.Initialized() << '\n';
}
