#include <gtest/gtest.h>

#include <data_sources/fdv2/mode_sources.hpp>
#include <flag_manager/flag_manager.hpp>

#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/logging/null_logger.hpp>

#include <boost/asio/io_context.hpp>

using launchdarkly::ContextBuilder;
using launchdarkly::client_side::flag_manager::FlagManager;
using launchdarkly::config::shared::GetConnectionModeName;

using namespace launchdarkly::client_side::data_sources;

namespace {

class ModeSourcesFixture : public ::testing::Test {
   public:
    ModeSourcesFixture()
        : logger_(launchdarkly::logging::NullLogger()),
          flag_manager_("sdk-key", logger_, 5, nullptr) {}

    static FDv2Config Defaults() {
        return launchdarkly::config::shared::Defaults<
            launchdarkly::config::shared::ClientSDK>::FDv2Config();
    }

    ModeSourceParams Params() {
        return ModeSourceParams{
            ioc_.get_executor(),
            logger_,
            "https://polling.example.com",
            "https://streaming.example.com",
            launchdarkly::config::shared::Defaults<
                launchdarkly::config::shared::ClientSDK>::HttpProperties(),
            ContextBuilder().Kind("user", "user-key").Build(),
            /* with_reasons= */ false,
            &flag_manager_.Cache()};
    }

   private:
    boost::asio::io_context ioc_;
    launchdarkly::Logger logger_;
    FlagManager flag_manager_;
};

}  // namespace

TEST_F(ModeSourcesFixture, StreamingModeInitializesFromCacheThenPolls) {
    auto sources =
        BuildModeSources(Defaults(), ConnectionMode::kStreaming, Params());

    ASSERT_EQ(2u, sources.initializers.size());
    EXPECT_TRUE(sources.initializers[0]->IsFromCache());
    EXPECT_FALSE(sources.initializers[1]->IsFromCache());
}

// Streaming is the primary tier and polling the fallback, so the SDK keeps
// receiving updates when a stream cannot be maintained.
TEST_F(ModeSourcesFixture, StreamingModeFallsBackToPolling) {
    auto sources =
        BuildModeSources(Defaults(), ConnectionMode::kStreaming, Params());

    ASSERT_EQ(2u, sources.synchronizers.size());
    EXPECT_EQ("FDv2 streaming synchronizer",
              sources.synchronizers[0]->Build()->Identity());
    EXPECT_EQ("FDv2 polling synchronizer",
              sources.synchronizers[1]->Build()->Identity());
}

TEST_F(ModeSourcesFixture, PollingModeInitializesFromCacheOnly) {
    auto sources =
        BuildModeSources(Defaults(), ConnectionMode::kPolling, Params());

    ASSERT_EQ(1u, sources.initializers.size());
    EXPECT_TRUE(sources.initializers[0]->IsFromCache());
    ASSERT_EQ(1u, sources.synchronizers.size());
    EXPECT_EQ("FDv2 polling synchronizer",
              sources.synchronizers[0]->Build()->Identity());
}

// Offline still loads persisted flags, but makes no requests.
TEST_F(ModeSourcesFixture, OfflineModeHasOnlyTheCacheInitializer) {
    auto sources =
        BuildModeSources(Defaults(), ConnectionMode::kOffline, Params());

    ASSERT_EQ(1u, sources.initializers.size());
    EXPECT_TRUE(sources.initializers[0]->IsFromCache());
    EXPECT_TRUE(sources.synchronizers.empty());
}

TEST_F(ModeSourcesFixture, UnconfiguredModeProducesNoSources) {
    auto config = launchdarkly::config::shared::Defaults<
        launchdarkly::config::shared::ClientSDK>::FDv2Config();
    config.modes.erase(ConnectionMode::kPolling);

    auto sources = BuildModeSources(config, ConnectionMode::kPolling, Params());

    EXPECT_TRUE(sources.initializers.empty());
    EXPECT_TRUE(sources.synchronizers.empty());
}

TEST_F(ModeSourcesFixture, AnOverriddenModeReplacesTheBuiltInPipeline) {
    auto config = launchdarkly::config::shared::Defaults<
        launchdarkly::config::shared::ClientSDK>::FDv2Config();
    config.modes[ConnectionMode::kStreaming] = FDv2Config::ModeDefinition{
        {FDv2Config::CacheConfig{}},
        {FDv2Config::StreamingConfig{std::chrono::seconds{1}, std::nullopt}},
        std::nullopt};

    auto sources =
        BuildModeSources(config, ConnectionMode::kStreaming, Params());

    EXPECT_EQ(1u, sources.initializers.size());
    ASSERT_EQ(1u, sources.synchronizers.size());
    EXPECT_EQ("FDv2 streaming synchronizer",
              sources.synchronizers[0]->Build()->Identity());
}

TEST(ConnectionModeTest, ModesAreNamedAsTheConfigurationSpellsThem) {
    EXPECT_STREQ("streaming",
                 GetConnectionModeName(ConnectionMode::kStreaming));
    EXPECT_STREQ("polling", GetConnectionModeName(ConnectionMode::kPolling));
    EXPECT_STREQ("offline", GetConnectionModeName(ConnectionMode::kOffline));
}

// Desktop starts in streaming, and provides no background mode.
TEST(FDv2ConfigTest, DefaultsProvideTheThreeDesktopModes) {
    auto const config = launchdarkly::config::shared::Defaults<
        launchdarkly::config::shared::ClientSDK>::FDv2Config();

    EXPECT_EQ(ConnectionMode::kStreaming, config.initial_mode);
    EXPECT_EQ(3u, config.modes.size());
    EXPECT_EQ(1u, config.modes.count(ConnectionMode::kStreaming));
    EXPECT_EQ(1u, config.modes.count(ConnectionMode::kPolling));
    EXPECT_EQ(1u, config.modes.count(ConnectionMode::kOffline));
}

TEST(FDv2ConfigTest, DefaultTimeoutsAreTwoAndFiveMinutes) {
    auto const config = launchdarkly::config::shared::Defaults<
        launchdarkly::config::shared::ClientSDK>::FDv2Config();

    EXPECT_EQ(std::chrono::seconds{120}, config.fallback_timeout);
    EXPECT_EQ(std::chrono::seconds{300}, config.recovery_timeout);
}

TEST(FDv2ConfigTest, ModesThatMakeRequestsConfigureAnFDv1Fallback) {
    auto const config = launchdarkly::config::shared::Defaults<
        launchdarkly::config::shared::ClientSDK>::FDv2Config();

    EXPECT_TRUE(
        config.modes.at(ConnectionMode::kStreaming).fdv1_fallback.has_value());
    EXPECT_TRUE(
        config.modes.at(ConnectionMode::kPolling).fdv1_fallback.has_value());
    EXPECT_FALSE(
        config.modes.at(ConnectionMode::kOffline).fdv1_fallback.has_value());
}
