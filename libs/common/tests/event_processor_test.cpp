#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <thread>
#include "config/client.hpp"
#include "console_backend.hpp"
#include "context_builder.hpp"
#include "events/client_events.hpp"
#include "events/detail/asio_event_processor.hpp"
#include "events/detail/summarizer.hpp"
#include "serialization/events/json_events.hpp"

using namespace launchdarkly::events::detail;

static std::chrono::system_clock::time_point TimeZero() {
    return std::chrono::system_clock::time_point{};
}

static std::chrono::system_clock::time_point Time1000() {
    return std::chrono::system_clock::from_time_t(1);
}

TEST(SummarizerTests, IsEmptyOnConstruction) {
    Summarizer summarizer;
    ASSERT_TRUE(summarizer.Empty());
}

TEST(SummarizerTests, DefaultConstructionUsesZeroStartTime) {
    Summarizer summarizer;
    ASSERT_EQ(summarizer.start_time(), TimeZero());
}

TEST(SummarizerTests, ExplicitStartTimeIsCorrect) {
    auto start = std::chrono::system_clock::now();
    Summarizer summarizer(start);
    ASSERT_EQ(summarizer.start_time(), start);
}

TEST(SummarizerTests, SummaryCounterUpdates) {
    using namespace launchdarkly::events::client;
    using namespace launchdarkly;
    Summarizer summarizer;

    auto const feature_key = "cat-food-amount";
    auto const feature_version = 1;
    auto const context = ContextBuilder().kind("cat", "shadow").build();
    auto const feature_value = Value(3);
    auto const feature_default = Value(1);
    auto const feature_variation = 0;

    auto const event = FeatureEventParams{
        TimeZero(),
        feature_key,
        context,
        EvaluationResult(
            feature_version, std::nullopt, false, false, std::nullopt,
            EvaluationDetailInternal(
                feature_value, feature_variation,
                EvaluationReason("FALLTHROUGH", std::nullopt, std::nullopt,
                                 std::nullopt, std::nullopt, false,
                                 std::nullopt))),
        feature_default,
    };

    auto const num_events = 10;
    for (size_t i = 0; i < num_events; i++) {
        summarizer.Update(event);
    }

    auto const& features = summarizer.features();
    auto const& cat_food = features.find(feature_key);
    ASSERT_TRUE(cat_food != features.end());

    auto const& counter = cat_food->second.counters.find(
        Summarizer::VariationKey(feature_version, feature_variation));
    ASSERT_TRUE(counter != cat_food->second.counters.end());

    ASSERT_EQ(counter->second.value().as_double(), feature_value.as_double());
    ASSERT_EQ(counter->second.count(), num_events);
}

TEST(SummarizerTests, JsonSerialization) {
    using namespace launchdarkly::events::client;
    using namespace launchdarkly;
    Summarizer summarizer;

    auto const feature_key = "cat-food-amount";
    auto const feature_version = 1;
    auto const context = ContextBuilder().kind("cat", "shadow").build();
    auto const feature_value = Value(3);
    auto const feature_default = Value(1);
    auto const feature_variation = 0;

    auto const event = FeatureEventParams{
        TimeZero(),
        feature_key,
        context,
        EvaluationResult(
            feature_version, std::nullopt, false, false, std::nullopt,
            EvaluationDetailInternal(
                feature_value, feature_variation,
                EvaluationReason("FALLTHROUGH", std::nullopt, std::nullopt,
                                 std::nullopt, std::nullopt, false,
                                 std::nullopt))),
        feature_default,
    };

    auto const num_events = 10;
    for (size_t i = 0; i < num_events; i++) {
        summarizer.Update(event);
    }
    auto json = boost::json::value_from(summarizer.Finish(Time1000()));
    auto expected = boost::json::parse(
        R"({"kind":"summary","startDate":0,"endDate":1000,"features":{"cat-food-amount":{"default":1E0,"contextKinds":["cat"],"counters":[{"version":1,"variation":0,"value":3E0,"count":10}]}}})");
    ASSERT_EQ(json, expected);
}

// This test is a temporary test that exists only to ensure the event processor
// compiles; it should be replaced by more robust tests (and contract tests.)
TEST(EventProcessorTests, ProcessorCompiles) {
    using namespace launchdarkly;

    Logger logger{std::make_unique<ConsoleBackend>(LogLevel::kDebug, "test")};
    boost::asio::io_context io;

    auto config = client::EventsBuilder()
                      .capacity(10)
                      .flush_interval(std::chrono::seconds(1))
                      .build();

    auto endpoints = client::Endpoints().build();

    events::detail::AsioEventProcessor ep(io.get_executor(), *config,
                                          *endpoints, "password", logger);
    std::thread t([&]() { io.run(); });

    auto c = launchdarkly::ContextBuilder().kind("org", "ld").build();
    ASSERT_TRUE(c.valid());

    auto ev = events::client::IdentifyEventParams{
        std::chrono::system_clock::now(),
        c,
    };

    for (std::size_t i = 0; i < 10; i++) {
        ep.AsyncSend(ev);
    }
    ep.AsyncClose();
    t.join();
}
