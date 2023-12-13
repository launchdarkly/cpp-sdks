#include <launchdarkly/bindings/c/logging/log_level.h>

#include <gtest/gtest.h>

TEST(LogLevelTests, LogLevelToString) {
    ASSERT_STREQ("debug", LDLogLevel_Name(LD_LOG_DEBUG, "unknown"));
    ASSERT_STREQ("info", LDLogLevel_Name(LD_LOG_INFO, "unknown"));
    ASSERT_STREQ("warn", LDLogLevel_Name(LD_LOG_WARN, "unknown"));
    ASSERT_STREQ("error", LDLogLevel_Name(LD_LOG_ERROR, "unknown"));
    ASSERT_STREQ("unknown",
                 LDLogLevel_Name(static_cast<LDLogLevel>(4141), "unknown"));
}

TEST(LogLevelTests, LogLevelToEnum) {
    constexpr auto unknown = static_cast<LDLogLevel>(4141);
    ASSERT_EQ(LD_LOG_DEBUG, LDLogLevel_Enum("debug", unknown));
    ASSERT_EQ(LD_LOG_INFO, LDLogLevel_Enum("info", unknown));
    ASSERT_EQ(LD_LOG_WARN, LDLogLevel_Enum("warn", unknown));
    ASSERT_EQ(LD_LOG_ERROR, LDLogLevel_Enum("error", unknown));
    ASSERT_EQ(unknown, LDLogLevel_Enum("potato", unknown));
}
