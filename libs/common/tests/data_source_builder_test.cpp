#include "config/detail/data_source_builder.hpp"
#include <gtest/gtest.h>
#include "value.hpp"

#include <boost/json.hpp>

using launchdarkly::config::detail::ClientSDK;
using launchdarkly::config::detail::DataSourceBuilder;
using launchdarkly::config::detail::PollingConfig;
using launchdarkly::config::detail::ServerSDK;
using launchdarkly::config::detail::StreamingConfig;

using ClientDataSource = DataSourceBuilder<ClientSDK>;
using ServerDataSource = DataSourceBuilder<ServerSDK>;

TEST(DataSourceBuilderTests, CanCreateStreamingClientConfig) {
    auto client_config =
        ClientDataSource()
            .with_reasons(true)
            .use_report(true)
            .method(ClientDataSource::Streaming().initial_reconnect_delay(
                std::chrono::milliseconds{1500}))
            .build();

    EXPECT_TRUE(client_config.use_report);
    EXPECT_TRUE(client_config.with_reasons);
    EXPECT_EQ(std::chrono::milliseconds{1500},
              boost::get<StreamingConfig>(client_config.method)
                  .initial_reconnect_delay);
}

TEST(DataSourceBuilderTests, CanCreatePollingClientConfig) {
    auto client_config = ClientDataSource()
                             .with_reasons(false)
                             .use_report(false)
                             .method(ClientDataSource::Polling().poll_interval(
                                 std::chrono::seconds{88000}))
                             .build();

    EXPECT_FALSE(client_config.use_report);
    EXPECT_FALSE(client_config.with_reasons);
    EXPECT_EQ(std::chrono::seconds{88000},
              boost::get<PollingConfig>(client_config.method).poll_interval);
}

TEST(DataSourceBuilderTests, CanCreateStreamingServerConfig) {
    auto server_config =
        ServerDataSource()
            .method(ServerDataSource::Streaming().initial_reconnect_delay(
                std::chrono::milliseconds{1500}))
            .build();

    EXPECT_EQ(std::chrono::milliseconds{1500},
              boost::get<StreamingConfig>(server_config.method)
                  .initial_reconnect_delay);
}

TEST(DataSourceBuilderTests, CanCreatePollingServerConfig) {
    auto server_config = ServerDataSource()
                             .method(ServerDataSource::Polling().poll_interval(
                                 std::chrono::seconds{30000}))
                             .build();

    EXPECT_EQ(std::chrono::seconds{30000},
              boost::get<PollingConfig>(server_config.method).poll_interval);
}
