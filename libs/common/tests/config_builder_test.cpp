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

TEST_F(ConfigBuilderTest,
       DefaultConstruction_ClientConfig_UsesDefaulDataSourceConfig) {
    using namespace launchdarkly::client;
    ConfigBuilder builder("sdk-123");
    Config cfg = builder.build(logger);

    EXPECT_FALSE(cfg.data_source_config.with_reasons);
    EXPECT_FALSE(cfg.data_source_config.use_report);
    // Should be streaming with a 1 second initial retry;
    EXPECT_EQ(std::chrono::milliseconds{1000},
              boost::get<launchdarkly::config::detail::StreamingConfig>(
                  cfg.data_source_config.method)
                  .initial_reconnect_delay);
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_ServerConfig_UsesDefaulDataSourceConfig) {
    using namespace launchdarkly::server;
    ConfigBuilder builder("sdk-123");
    Config cfg = builder.build(logger);

    // Should be streaming with a 1 second initial retry;
    EXPECT_EQ(std::chrono::milliseconds{1000},
              boost::get<launchdarkly::config::detail::StreamingConfig>(
                  cfg.data_source_config.method)
                  .initial_reconnect_delay);
}

TEST_F(ConfigBuilderTest, ServerConfig_CanSetDataSource) {
    using namespace launchdarkly::server;
    ConfigBuilder builder("sdk-123");

    builder.data_source(ConfigBuilder::DataSourceBuilder().method(
        ConfigBuilder::DataSourceBuilder::Streaming().initial_reconnect_delay(
            std::chrono::milliseconds{5000})));

    Config cfg = builder.build(logger);

    EXPECT_EQ(std::chrono::milliseconds{5000},
              boost::get<launchdarkly::config::detail::StreamingConfig>(
                  cfg.data_source_config.method)
                  .initial_reconnect_delay);
}

TEST_F(ConfigBuilderTest, ClientConfig_CanSetDataSource) {
    using namespace launchdarkly::client;
    ConfigBuilder builder("sdk-123");

    builder.data_source(
        ConfigBuilder::DataSourceBuilder()
            .method(
                ConfigBuilder::DataSourceBuilder::Streaming()
                    .initial_reconnect_delay(std::chrono::milliseconds{5000}))
            .use_report(true)
            .with_reasons(true));

    Config cfg = builder.build(logger);

    EXPECT_TRUE(cfg.data_source_config.use_report);
    EXPECT_TRUE(cfg.data_source_config.with_reasons);
    EXPECT_EQ(std::chrono::milliseconds{5000},
              boost::get<launchdarkly::config::detail::StreamingConfig>(
                  cfg.data_source_config.method)
                  .initial_reconnect_delay);
}
