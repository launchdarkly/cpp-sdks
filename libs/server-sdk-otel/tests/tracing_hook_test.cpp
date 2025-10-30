/**
 * @file tracing_hook_test.cpp
 * @brief Unit tests for OpenTelemetry tracing hook
 */

#include <gtest/gtest.h>

#include <launchdarkly/server_side/integrations/otel/tracing_hook.hpp>

namespace launchdarkly::server_side::integrations::otel {

// Basic construction tests
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

// Options builder tests
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

TEST(TracingHookOptionsBuilderTest, IncludeValueDefaultsFalse) {
    auto options = TracingHookOptionsBuilder()
                       .CreateSpans(true)
                       .Build();

    EXPECT_FALSE(options.IncludeValue());
    EXPECT_TRUE(options.CreateSpans());
}

TEST(TracingHookOptionsBuilderTest, CreateSpansDefaultsFalse) {
    auto options = TracingHookOptionsBuilder()
                       .IncludeValue(true)
                       .Build();

    EXPECT_TRUE(options.IncludeValue());
    EXPECT_FALSE(options.CreateSpans());
}

TEST(TracingHookOptionsBuilderTest, CanSetMultipleOptions) {
    auto options = TracingHookOptionsBuilder()
                       .IncludeValue(true)
                       .CreateSpans(true)
                       .EnvironmentId("dev")
                       .Build();

    EXPECT_TRUE(options.IncludeValue());
    EXPECT_TRUE(options.CreateSpans());
    ASSERT_TRUE(options.EnvironmentId().has_value());
    EXPECT_EQ(options.EnvironmentId().value(), "dev");
}

TEST(TracingHookOptionsBuilderTest, EnvironmentIdIsOptional) {
    auto options1 = TracingHookOptionsBuilder()
                        .IncludeValue(true)
                        .Build();
    EXPECT_FALSE(options1.EnvironmentId().has_value());

    auto options2 = TracingHookOptionsBuilder()
                        .CreateSpans(true)
                        .Build();
    EXPECT_FALSE(options2.EnvironmentId().has_value());
}

TEST(TracingHookOptionsBuilderTest, BuilderIsReusable) {
    auto builder = TracingHookOptionsBuilder()
                       .IncludeValue(true)
                       .CreateSpans(true);

    auto options1 = builder.Build();
    auto options2 = builder.EnvironmentId("test").Build();

    EXPECT_TRUE(options1.IncludeValue());
    EXPECT_TRUE(options2.IncludeValue());
    EXPECT_FALSE(options1.EnvironmentId().has_value());
    EXPECT_TRUE(options2.EnvironmentId().has_value());
}

// Metadata tests
TEST(TracingHookTest, MetadataNameIsCorrect) {
    TracingHook hook;
    EXPECT_EQ(hook.Metadata().Name(), "LaunchDarkly OpenTelemetry Tracing Hook");
}

TEST(TracingHookTest, MetadataIsConsistent) {
    auto options = TracingHookOptionsBuilder()
                       .IncludeValue(true)
                       .CreateSpans(true)
                       .EnvironmentId("production")
                       .Build();
    TracingHook hook(options);

    // Metadata should be the same regardless of options
    EXPECT_EQ(hook.Metadata().Name(), "LaunchDarkly OpenTelemetry Tracing Hook");
}

}  // namespace launchdarkly::server_side::integrations::otel
