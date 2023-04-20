#include <gtest/gtest.h>
#include <launchdarkly/client_side/api.hpp>
#include "context_builder.hpp"

using namespace launchdarkly;

TEST(ClientTest, ConstructClientWithConfig) {
    tl::expected<client_side::Config, Error> config =
        client_side::ConfigBuilder("sdk-123").Build();

    ASSERT_TRUE(config);

    auto context = ContextBuilder().kind("cat", "shadow").build();

    client_side::Client client(std::move(*config), context);

    ASSERT_TRUE(client.AllFlags().empty());
    ASSERT_TRUE(client.BoolVariation("cat-food", true));
}
