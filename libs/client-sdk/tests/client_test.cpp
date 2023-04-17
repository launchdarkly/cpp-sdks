#include <gtest/gtest.h>
#include <launchdarkly/client_side/api.hpp>
#include "context_builder.hpp"
using namespace launchdarkly;

TEST(ClientTest, ConstructClientWithConfig) {
    tl::expected<client::Config, Error> config =
        client::ConfigBuilder("sdk-123").build();

    ASSERT_TRUE(config);

    auto context = ContextBuilder().kind("cat", "shadow").build();

    client_side::Client client(std::move(*config), context);

    ASSERT_TRUE(client.AllFlags().empty());
    ASSERT_TRUE(client.BoolVariation("cat-food", true));
}
