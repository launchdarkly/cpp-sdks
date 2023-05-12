#include <gtest/gtest.h>
#include <launchdarkly/config/client.hpp>
#include <launchdarkly/config/detail/sdks.hpp>
#include <launchdarkly/config/server.hpp>
#include <launchdarkly/value.hpp>

#include <boost/json.hpp>
#include <boost/url.hpp>

using namespace launchdarkly;

TEST(DataSourceBuilderTests, CanCreateStreamingClientConfig) {
    auto client_config =
        client_side::DataSourceBuilder()
            .WithReasons(true)
            .UseReport(true)
            .Method(client_side::DataSourceBuilder::Streaming()
                        .InitialReconnectDelay(std::chrono::milliseconds{1500}))
            .Build();

    EXPECT_TRUE(client_config.use_report);
    EXPECT_TRUE(client_config.with_reasons);
    EXPECT_EQ(
        std::chrono::milliseconds{1500},
        std::get<
            config::detail::built::StreamingConfig<config::detail::ClientSDK>>(
            client_config.method)
            .initial_reconnect_delay);
}

TEST(DataSourceBuilderTests, CanCreatePollingClientConfig) {
    auto client_config =
        client_side::DataSourceBuilder()
            .WithReasons(false)
            .UseReport(false)
            .Method(client_side::DataSourceBuilder::Polling().PollInterval(
                std::chrono::seconds{88000}))
            .Build();

    EXPECT_FALSE(client_config.use_report);
    EXPECT_FALSE(client_config.with_reasons);
    EXPECT_EQ(
        std::chrono::seconds{88000},
        std::get<
            config::detail::built::PollingConfig<config::detail::ClientSDK>>(
            client_config.method)
            .poll_interval);
}

TEST(DataSourceBuilderTests, CanCreateStreamingServerConfig) {
    auto server_config =
        server_side::DataSourceBuilder()
            .Method(server_side::DataSourceBuilder::Streaming()
                        .InitialReconnectDelay(std::chrono::milliseconds{1500}))
            .Build();

    EXPECT_EQ(
        std::chrono::milliseconds{1500},
        std::get<
            config::detail::built::StreamingConfig<config::detail::ServerSDK>>(
            server_config.method)
            .initial_reconnect_delay);
}

TEST(DataSourceBuilderTests, CanCreatePollingServerConfig) {
    auto server_config =
        server_side::DataSourceBuilder()
            .Method(server_side::DataSourceBuilder::Polling().PollInterval(
                std::chrono::seconds{30000}))
            .Build();

    EXPECT_EQ(
        std::chrono::seconds{30000},
        std::get<
            config::detail::built::PollingConfig<config::detail::ServerSDK>>(
            server_config.method)
            .poll_interval);
}
