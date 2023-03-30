#include <gtest/gtest.h>

#include "config/client.hpp"
#include "config/server.hpp"

class ConfigBuilderTest : public testing::Test {};

TEST(ConfigBuilderTest, DefaultConstruction_ClientConfig) {
    using namespace launchdarkly::client;
    ConfigBuilder builder("sdk-123");
    Config cfg = builder.build();
    ASSERT_EQ(cfg.sdk_key, "sdk-123");
}

TEST(ConfigBuilderTest, DefaultConstruction_ServerConfig) {
    using namespace launchdarkly::server;
    ConfigBuilder builder("sdk-123");
    Config cfg = builder.build();
    ASSERT_EQ(cfg.sdk_key, "sdk-123");
}

TEST(ConfigBuilderTest, DefaultConstruction_UsesDefaultEndpointsIfNotSupplied) {
    using namespace launchdarkly::client;
    ConfigBuilder builder("sdk-123");
    Config cfg = builder.build();
    ASSERT_EQ(cfg.service_endpoints_builder, ConfigBuilder::EndpointsBuilder());
}

TEST(ConfigBuilderTest,
     DefaultConstruction_UsesDefaultOfflineModeIfNotSupplied) {
    using namespace launchdarkly::client;
    ConfigBuilder builder("sdk-123");
    Config cfg = builder.build();
    ASSERT_FALSE(cfg.offline);
}

// This test should exercise all of the config options.
TEST(ConfigBuilderTest, CustomBuilderReflectsChanges) {
    using namespace launchdarkly::client;
    auto config =
        ConfigBuilder("sdk-123")
            .offline(true)
            .service_endpoints(Endpoints().relay_proxy("foo"))
            .application_info(
                ApplicationInfo().app_identifier("bar").app_version("baz"))
            .build();

    ASSERT_EQ(config.sdk_key, "sdk-123");
    ASSERT_TRUE(config.offline);
    ASSERT_EQ(config.service_endpoints_builder, Endpoints().relay_proxy("foo"));
    ASSERT_EQ(config.application_tag,
              "application-id/bar application-version/baz");
}
