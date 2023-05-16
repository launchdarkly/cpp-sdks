#include <gtest/gtest.h>

#include <launchdarkly/config/shared/builders/logging_builder.hpp>

using launchdarkly::LogLevel;
using launchdarkly::config::shared::builders::LoggingBuilder;

TEST(LoggingBuilderTests, DefaultLoggingConfiguration) {
    auto config = LoggingBuilder().Build();
    ASSERT_FALSE(config.disable_logging);
    ASSERT_EQ("LaunchDarkly", config.tag);
    ASSERT_EQ(LogLevel::kInfo, config.level);
    ASSERT_EQ(std::shared_ptr<launchdarkly::ILogBackend>(), config.backend);
}

TEST(LoggingBuilderTests, ConfigureBasicLogger) {
    auto config = LoggingBuilder()
                      .Logging(LoggingBuilder::BasicLogging()
                                   .Level(LogLevel::kWarn)
                                   .Tag("Potato"))
                      .Build();
    ASSERT_FALSE(config.disable_logging);
    ASSERT_EQ("Potato", config.tag);
    ASSERT_EQ(LogLevel::kWarn, config.level);
    ASSERT_EQ(std::shared_ptr<launchdarkly::ILogBackend>(), config.backend);
}

TEST(LoggingBuilderTests, ConfigureNoLogging) {
    auto config = LoggingBuilder().Logging(LoggingBuilder::NoLogging()).Build();
    ASSERT_TRUE(config.disable_logging);
}

class CustomBackend : public launchdarkly::ILogBackend {
   public:
    bool Enabled(LogLevel level) noexcept override { return false; }
    void Write(LogLevel level, std::string message) noexcept override {}
};

TEST(LoggingBuilderTests, ConfigureCustomLogging) {
    auto backend = std::make_shared<CustomBackend>();
    auto config = LoggingBuilder()
                      .Logging(LoggingBuilder::CustomLogging().Backend(backend))
                      .Build();
    ASSERT_FALSE(config.disable_logging);
    ASSERT_EQ(backend, config.backend);
}

TEST(LoggingBuilderTests, NoBackendSet) {
    auto backend = std::make_shared<CustomBackend>();
    auto config = LoggingBuilder().Build();

    ASSERT_FALSE(config.disable_logging);
    ASSERT_EQ("LaunchDarkly", config.tag);
    ASSERT_EQ(LogLevel::kInfo, config.level);
    ASSERT_EQ(std::shared_ptr<launchdarkly::ILogBackend>(), config.backend);
}
