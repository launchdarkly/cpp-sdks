/**
 * @file tracing_hook_test.cpp
 * @brief Unit tests for OpenTelemetry tracing hook
 */

#include <gtest/gtest.h>

#include <launchdarkly/server_side/integrations/otel/tracing_hook.hpp>

namespace launchdarkly::server_side::integrations::otel {

// Basic smoke test
TEST(TracingHookTest, ConstructsWithDefaultOptions) {
    TracingHook hook;
    EXPECT_EQ(hook.Metadata().Name(), "LaunchDarkly OpenTelemetry Tracing Hook");
}

TEST(TracingHookTest, ConstructsWithCustomOptions) {
    auto options = TracingHookOptionsBuilder()
                       .IncludeValue(true)
                       .CreateSpans(true)
                       .EnvironmentId("test-env")
                       .Build();

    TracingHook hook(options);
    EXPECT_EQ(hook.Metadata().Name(), "LaunchDarkly OpenTelemetry Tracing Hook");
}

TEST(TracingHookOptionsBuilderTest, BuildsDefaultOptions) {
    auto options = TracingHookOptionsBuilder().Build();

    EXPECT_FALSE(options.IncludeValue());
    EXPECT_FALSE(options.CreateSpans());
    EXPECT_FALSE(options.EnvironmentId().has_value());
}

TEST(TracingHookOptionsBuilderTest, BuildsWithAllOptions) {
    auto options = TracingHookOptionsBuilder()
                       .IncludeValue(true)
                       .CreateSpans(true)
                       .EnvironmentId("production")
                       .Build();

    EXPECT_TRUE(options.IncludeValue());
    EXPECT_TRUE(options.CreateSpans());
    ASSERT_TRUE(options.EnvironmentId().has_value());
    EXPECT_EQ(options.EnvironmentId().value(), "production");
}

TEST(TracingHookOptionsBuilderTest, IgnoresEmptyEnvironmentId) {
    auto options = TracingHookOptionsBuilder()
                       .EnvironmentId("")
                       .Build();

    EXPECT_FALSE(options.EnvironmentId().has_value());
}

TEST(TracingHookOptionsBuilderTest, ChainsMethods) {
    auto options = TracingHookOptionsBuilder()
                       .IncludeValue(true)
                       .CreateSpans(false)
                       .EnvironmentId("staging")
                       .Build();

    EXPECT_TRUE(options.IncludeValue());
    EXPECT_FALSE(options.CreateSpans());
    EXPECT_EQ(options.EnvironmentId().value(), "staging");
}

// TODO: Add integration tests with mock OpenTelemetry components
// TODO: Add tests for BeforeEvaluation and AfterEvaluation
// TODO: Add tests for span event creation
// TODO: Add tests for span creation when enabled
// TODO: Add tests for HookContext span injection

}  // namespace launchdarkly::server_side::integrations::otel
