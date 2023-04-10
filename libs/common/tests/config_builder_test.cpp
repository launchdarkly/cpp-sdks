#include <gtest/gtest.h>

#include "config/client.hpp"
#include "config/server.hpp"
#include "null_logger.hpp"

class ConfigBuilderTest
    : public ::testing::
          Test {  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
   protected:
    launchdarkly::Logger logger;
    ConfigBuilderTest() : logger(NullLogger()) {}
};

TEST_F(ConfigBuilderTest, DefaultConstruction_ClientConfig) {
    using namespace launchdarkly::client;
    ConfigBuilder builder("sdk-123");
    Config cfg = builder.build(logger);
    ASSERT_EQ(cfg.sdk_key, "sdk-123");
}

TEST_F(ConfigBuilderTest, DefaultConstruction_ServerConfig) {
    using namespace launchdarkly::server;
    ConfigBuilder builder("sdk-123");
    Config cfg = builder.build(logger);
    ASSERT_EQ(cfg.sdk_key, "sdk-123");
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_UsesDefaultEndpointsIfNotSupplied) {
    using namespace launchdarkly::client;
    ConfigBuilder builder("sdk-123");
    Config cfg = builder.build(logger);
    ASSERT_EQ(cfg.service_endpoints_builder, ConfigBuilder::EndpointsBuilder());
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_UsesDefaultOfflineModeIfNotSupplied) {
    using namespace launchdarkly::client;
    ConfigBuilder builder("sdk-123");
    Config cfg = builder.build(logger);
    ASSERT_FALSE(cfg.offline);
}

// This test should exercise all of the config options.
TEST_F(ConfigBuilderTest, CustomBuilderReflectsChanges) {
    using namespace launchdarkly::client;
    auto config =
        ConfigBuilder("sdk-123")
            .offline(true)
            .service_endpoints(Endpoints().relay_proxy("foo"))
            .application_info(
                ApplicationInfo().app_identifier("bar").app_version("baz"))
            .build(logger);

    ASSERT_EQ(config.sdk_key, "sdk-123");
    ASSERT_TRUE(config.offline);
    ASSERT_EQ(config.service_endpoints_builder, Endpoints().relay_proxy("foo"));
    ASSERT_EQ(config.application_tag,
              "application-id/bar application-version/baz");
}
