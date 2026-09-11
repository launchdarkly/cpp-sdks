#include <gtest/gtest.h>

#include <data_sources/fdv2/polling_initializer.hpp>
#include <data_sources/fdv2/polling_synchronizer.hpp>

#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <boost/asio/io_context.hpp>

using namespace launchdarkly;
using namespace launchdarkly::client_side::data_sources;
using namespace std::chrono_literals;

static Logger MakeNullLogger() {
    struct NullBackend : ILogBackend {
        bool Enabled(LogLevel) noexcept override { return false; }
        void Write(LogLevel, std::string) noexcept override {}
    };
    return Logger{std::make_shared<NullBackend>()};
}

static FDv2RequestConfig MakeConfig(std::string base_url) {
    return FDv2RequestConfig{
        std::move(base_url),
        config::shared::Defaults<config::shared::ClientSDK>::HttpProperties(),
        R"({"kind":"user","key":"user-key"})", FDv2ContextTransport::kGetPath,
        false};
}

TEST(FDv2PollingInitializerTests, UnparseableEndpointIsATerminalError) {
    boost::asio::io_context ioc;
    auto logger = MakeNullLogger();
    FDv2PollingInitializer initializer(ioc.get_executor(), logger,
                                       MakeConfig("not a url"));

    auto future = initializer.Run();

    ASSERT_TRUE(future.IsFinished());
    EXPECT_TRUE(std::holds_alternative<FDv2SourceResult::TerminalError>(
        future.GetResult()->value));
}

TEST(FDv2PollingSynchronizerTests, NextRespectsTheIntervalSinceTheLastPoll) {
    boost::asio::io_context ioc;
    auto logger = MakeNullLogger();
    FDv2PollingSynchronizer synchronizer(ioc.get_executor(), logger,
                                         MakeConfig("http://example.com"), 30s,
                                         std::chrono::steady_clock::now());

    auto future = synchronizer.Next(data_model::Selector{});

    // The interval has not elapsed, so no request has been made and only the
    // interval timer or Close can resolve the future.
    EXPECT_FALSE(future.IsFinished());
}

TEST(FDv2PollingSynchronizerTests, CloseUnblocksAPendingNext) {
    boost::asio::io_context ioc;
    auto logger = MakeNullLogger();
    FDv2PollingSynchronizer synchronizer(ioc.get_executor(), logger,
                                         MakeConfig("http://example.com"), 30s,
                                         std::chrono::steady_clock::now());

    auto future = synchronizer.Next(data_model::Selector{});
    synchronizer.Close();

    ASSERT_TRUE(future.IsFinished());
    EXPECT_TRUE(std::holds_alternative<FDv2SourceResult::Shutdown>(
        future.GetResult()->value));
}

TEST(FDv2PollingSynchronizerTests, NextAfterCloseIsShutdown) {
    boost::asio::io_context ioc;
    auto logger = MakeNullLogger();
    FDv2PollingSynchronizer synchronizer(ioc.get_executor(), logger,
                                         MakeConfig("http://example.com"), 30s,
                                         std::nullopt);

    synchronizer.Close();
    auto future = synchronizer.Next(data_model::Selector{});

    ASSERT_TRUE(future.IsFinished());
    EXPECT_TRUE(std::holds_alternative<FDv2SourceResult::Shutdown>(
        future.GetResult()->value));
}

TEST(FDv2PollingSynchronizerTests, IntervalIsClampedToTheMinimum) {
    boost::asio::io_context ioc;
    auto logger = MakeNullLogger();
    auto const min_interval =
        launchdarkly::config::shared::Defaults<
            launchdarkly::config::shared::ClientSDK>::PollingConfig()
            .min_polling_interval;
    FDv2PollingSynchronizer synchronizer(
        ioc.get_executor(), logger, MakeConfig("http://example.com"), 1s,
        std::chrono::steady_clock::now() - (min_interval - 5s));

    auto future = synchronizer.Next(data_model::Selector{});

    // With the configured 1s interval the poll would already be due. Clamped
    // to the minimum it is not.
    EXPECT_FALSE(future.IsFinished());
}
