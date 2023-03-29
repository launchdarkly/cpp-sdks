#include <gtest/gtest.h>

#include "config/client.hpp"

class ConfigBuilderTest : public testing::Test {};

using namespace launchdarkly::client;

TEST(ConfigBuilderTest, DefaultConfig) {
    ConfigBuilder builder("sdk-123");
    Config cfg = builder.build();
    ASSERT_EQ(cfg.sdk_key, "sdk-123");

}
