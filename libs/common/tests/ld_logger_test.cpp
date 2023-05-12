#include <gtest/gtest.h>
#include <launchdarkly/console_backend.hpp>
#include <launchdarkly/log_backend.hpp>
#include <launchdarkly/logger.hpp>
#include "ostream_tester.hpp"

using launchdarkly::ConsoleBackend;
using launchdarkly::Logger;
using launchdarkly::LogLevel;

using Messages = std::map<LogLevel, std::vector<std::string>>;

class TestLogBackend : public launchdarkly::ILogBackend {
   public:
    TestLogBackend(LogLevel level, std::string name, Messages& messages)
        : level_(level), name_(std::move(name)), messages(messages) {}

    bool enabled(LogLevel level) override { return level >= level_; }

    void write(LogLevel level, std::string message) override {
        messages[level].push_back(message);
    }

    // Public for test purposes.
    std::map<LogLevel, std::vector<std::string>>& messages;  // NOLINT

   private:
    LogLevel level_;
    std::string name_;
};

class ConstTester {
   public:
    ConstTester(Logger const& logger) : logger_(logger) {}

    void do_log() const { LD_LOG(logger_, LogLevel::kInfo) << "const log"; }

   private:
    Logger const& logger_;
};

TEST(LDLoggerTest, CanMakeALogger) {
    Messages messages;
    EXPECT_NO_THROW(Logger logger(std::make_unique<TestLogBackend>(
        LogLevel::kDebug, "TestLogger", messages)));
}

class LogLevelParameterizedTestFixture
    : public ::testing::TestWithParam<LogLevel> {};

TEST_P(LogLevelParameterizedTestFixture, WritesToBackend) {
    auto level = GetParam();
    Messages messages;
    Logger logger(
        std::make_unique<TestLogBackend>(level, "TestLogger", messages));

    LD_LOG(logger, level) << "the message";

    EXPECT_EQ(1, messages.count(level));
    EXPECT_EQ("the message", messages[level][0]);
}

TEST_P(LogLevelParameterizedTestFixture, GetsEnabledFromBackend) {
    auto level = GetParam();
    Messages messages;
    Logger logger(
        std::make_unique<TestLogBackend>(level, "TestLogger", messages));

    std::vector<LogLevel> other_levels{LogLevel::kDebug, LogLevel::kInfo,
                                       LogLevel::kWarn, LogLevel::kError};

    other_levels.erase(std::remove_if(other_levels.begin(), other_levels.end(),
                                      [level](LogLevel const& other) {
                                          return other >= level;
                                      }),
                       other_levels.end());

    EXPECT_TRUE(logger.enabled(level));

    for (auto other : other_levels) {
        EXPECT_FALSE(logger.enabled(other));
    }
}

INSTANTIATE_TEST_SUITE_P(LDLoggerTest,
                         LogLevelParameterizedTestFixture,
                         testing::Values(LogLevel::kDebug,
                                         LogLevel::kInfo,
                                         LogLevel::kWarn,
                                         LogLevel::kError),
                         [](testing::TestParamInfo<LogLevel> const& info) {
                             return launchdarkly::GetLogLevelName(info.param,
                                                                  "unknown");
                         });

TEST(LDLoggerTest, UsesOstreamForEnabledLevel) {
    Messages messages;
    Logger logger(std::make_unique<TestLogBackend>(LogLevel::kDebug,
                                                   "TestLogger", messages));

    bool was_converted = false;
    OstreamTester tester(was_converted);

    LD_LOG(logger, LogLevel::kDebug) << tester;
    EXPECT_TRUE(was_converted);
}

TEST(LDLoggerTest, DoesNotUseOstreamForDisabledLevel) {
    Messages messages;
    Logger logger(std::make_unique<TestLogBackend>(LogLevel::kError,
                                                   "TestLogger", messages));

    bool was_converted = false;
    OstreamTester tester(was_converted);

    LD_LOG(logger, LogLevel::kDebug) << tester;
    EXPECT_FALSE(was_converted);
}

TEST(LDLoggerTest, LogFromConstMethod) {
    Messages messages;
    Logger logger(std::make_unique<TestLogBackend>(LogLevel::kDebug,
                                                   "TestLogger", messages));

    ConstTester const_tester(logger);
    const_tester.do_log();

    EXPECT_EQ(1, messages.count(LogLevel::kInfo));
    EXPECT_EQ("const log", messages[LogLevel::kInfo][0]);
}
