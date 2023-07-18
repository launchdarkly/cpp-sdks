#include <gtest/gtest.h>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/server_side/client.hpp>
#include <map>

using namespace launchdarkly;
using namespace launchdarkly::server_side;

TEST(ClientTestT, ClientConstructedWithMinimalConfigAndContextT) {
    tl::expected<Config, Error> config = ConfigBuilder("sdk-123").Build();
    ASSERT_TRUE(config);

    Client client(*config);

    char const* version = client.Version();
    ASSERT_TRUE(version);
    ASSERT_STREQ(version, "0.1.0");  // {x-release-please-version}
}
