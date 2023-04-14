#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "config/client.hpp"
#include "context_builder.hpp"
#include "events/client_events.hpp"
#include "events/common_events.hpp"
#include "events/detail/summarizer.hpp"
#include "serialization/events/json_events.hpp"

using namespace launchdarkly::events;
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
    auto start = std::chrono::system_clock::from_time_t(12345);
    Summarizer summarizer(start);
    ASSERT_EQ(summarizer.start_time(), start);
}

struct EvaluationParams {
    std::string feature_key;
    Version feature_version;
    VariationIndex feature_variation;
    Value feature_value;
    Value feature_default;
};
struct SummaryTestCase {
    // The pair is (parameters of the eval, number of times evaluated).
    std::string test_name;
    std::vector<std::pair<EvaluationParams, int32_t>> evaluations;
};

class SummaryCounterTestsFixture
    : public ::testing::TestWithParam<SummaryTestCase> {};

TEST_P(SummaryCounterTestsFixture, EventsAreCounted) {
    using namespace launchdarkly::events::client;
    using namespace launchdarkly;
    Summarizer summarizer;

    auto test_params = GetParam();

    auto const context = ContextBuilder().kind("cat", "shadow").build();

    for (auto const& eval : test_params.evaluations) {
        auto params = eval.first;
        auto count = eval.second;

        auto const event = FeatureEventParams{
            TimeZero(),
            params.feature_key,
            context,
            EvaluationResult(
                params.feature_version, std::nullopt, false, false,
                std::nullopt,
                EvaluationDetailInternal(
                    params.feature_value, params.feature_variation,
                    EvaluationReason("FALLTHROUGH", std::nullopt, std::nullopt,
                                     std::nullopt, std::nullopt, false,
                                     std::nullopt))),
            params.feature_default,
        };

        for (size_t i = 0; i < count; i++) {
            summarizer.Update(event);
        }
    }

    for (auto const& eval : test_params.evaluations) {
        auto params = eval.first;
        auto count = eval.second;

        auto const& features = summarizer.features();
        auto const& feature = features.find(params.feature_key);
        if (count == 0) {
            ASSERT_TRUE(feature == features.end());
            continue;
        } else {
            ASSERT_TRUE(feature != features.end());
        }

        auto const& counter =
            feature->second.counters.find(Summarizer::VariationKey(
                params.feature_version, params.feature_variation));
        ASSERT_TRUE(counter != feature->second.counters.end());

        ASSERT_EQ(counter->second.value(), params.feature_value);
        ASSERT_EQ(counter->second.count(), count);
    }
}

INSTANTIATE_TEST_CASE_P(
    SummaryCounterTests,
    SummaryCounterTestsFixture,
    ::testing::Values(
        SummaryTestCase{"no evaluations means no counter updates",
                        {{EvaluationParams{}, 0}}},
        SummaryTestCase{"single evaluation of a feature",
                        {{EvaluationParams{
                              "cat-food-amount",
                              Version(1),
                              VariationIndex(0),
                              Value(150.0),
                              Value(100.0),
                          },
                          1}}},
        SummaryTestCase{"100,000 evaluations of same feature",
                        {{EvaluationParams{
                              "cat-food-amount",
                              Version(1),
                              VariationIndex(0),
                              Value(150.0),
                              Value(100.0),
                          },
                          100000}}},
        SummaryTestCase{"two features, one evaluation each",
                        {{EvaluationParams{
                              "cat-food-amount",
                              Version(1),
                              VariationIndex(0),
                              Value(150.0),
                              Value(100.0),
                          },
                          1},
                         {EvaluationParams{
                              "cat-water-amount",
                              Version(1),
                              VariationIndex(0),
                              Value(500.0),
                              Value(250.0),
                          },
                          1}}},
        SummaryTestCase{"two features, 100,000 evaluations each",
                        {{EvaluationParams{
                              "cat-food-amount",
                              Version(1),
                              VariationIndex(0),
                              Value(150.0),
                              Value(100.0),
                          },
                          100000},
                         {EvaluationParams{
                              "cat-water-amount",
                              Version(1),
                              VariationIndex(0),
                              Value(500.0),
                              Value(250.0),
                          },
                          100000}}},
        SummaryTestCase{"one feature, different variations",
                        {{EvaluationParams{
                              "cat-food-amount",
                              Version(1),
                              VariationIndex(0),
                              Value(100.0),
                              Value(100.0),
                          },
                          1},
                         {EvaluationParams{
                              "cat-food-amount",
                              Version(1),
                              VariationIndex(1),
                              Value(200.0),
                              Value(100.0),
                          },
                          1},
                         {EvaluationParams{
                              "cat-food-amount",
                              Version(1),
                              VariationIndex(2),
                              Value(300.0),
                              Value(100.0),
                          },
                          1}}},
        SummaryTestCase{"one feature, same variation but different versions",
                        {{EvaluationParams{
                              "cat-food-amount",
                              Version(1),
                              VariationIndex(0),
                              Value(100.0),
                              Value(100.0),
                          },
                          1},
                         {EvaluationParams{
                              "cat-food-amount",
                              Version(2),
                              VariationIndex(0),
                              Value(200.0),
                              Value(100.0),
                          },
                          1},
                         {EvaluationParams{
                              "cat-food-amount",
                              Version(3),
                              VariationIndex(0),
                              Value(300.0),
                              Value(100.0),
                          },
                          1}}},
        SummaryTestCase{"one feature, different variation/version combos",
                        {{EvaluationParams{
                              "cat-food-amount",
                              Version(0),
                              VariationIndex(0),
                              Value(0),
                              Value(0),
                          },
                          1},
                         {EvaluationParams{
                              "cat-food-amount",
                              Version(1),
                              VariationIndex(0),
                              Value(1),
                              Value(1),
                          },
                          1},
                         {EvaluationParams{
                              "cat-food-amount",
                              Version(0),
                              VariationIndex(1),
                              Value(2),
                              Value(2),
                          },
                          1},
                         {EvaluationParams{
                              "cat-food-amount",
                              Version(1),
                              VariationIndex(1),
                              Value(3),
                              Value(3),
                          },
                          1}}}));

TEST(SummarizerTests, MissingFlagCreatesCounterUsingDefaultValue) {
    using namespace launchdarkly::events::client;
    using namespace launchdarkly;
    Summarizer summarizer;

    auto const feature_key = "cat-food-amount";
    auto const context = ContextBuilder().kind("cat", "shadow").build();
    auto const feature_default = Value(1);

    auto const event = FeatureEventParams{
        TimeZero(),
        feature_key,
        context,
        EvaluationResult(
            0, std::nullopt, false, false, std::nullopt,
            EvaluationDetailInternal(
                Value(), std::nullopt,
                EvaluationReason("ERROR", "FLAG_NOT_FOUND", std::nullopt,
                                 std::nullopt, std::nullopt, false,
                                 std::nullopt))),
        feature_default,
    };

    summarizer.Update(event);

    auto const& features = summarizer.features();
    auto const& feature = features.find(feature_key);

    ASSERT_TRUE(feature != features.end());

    auto const& default_counter =
        feature->second.counters.find(Summarizer::VariationKey());

    ASSERT_TRUE(default_counter != feature->second.counters.end());

    ASSERT_EQ(default_counter->second.value(), feature_default);
    ASSERT_EQ(default_counter->second.count(), 1);
}

TEST(SummarizerTests, JsonSerialization) {
    using namespace launchdarkly::events::client;
    using namespace launchdarkly;
    Summarizer summarizer;

    auto evaluate = [&](char const* feature_key, std::int32_t count) {
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

        for (size_t i = 0; i < count; i++) {
            summarizer.Update(event);
        }
    };

    evaluate("cat-food-amount", 1);
    evaluate("cat-water-amount", 10);
    evaluate("cat-toy-amount", 10000);

    auto json = boost::json::value_from(summarizer.Finish(Time1000()));
    auto expected = boost::json::parse(
        R"({"kind":"summary","startDate":0,"endDate":1000,"features":{"cat-toy-amount":{"default":1E0,"contextKinds":["cat"],"counters":[{"version":1,"variation":0,"value":3E0,"count":10000}]},"cat-water-amount":{"default":1E0,"contextKinds":["cat"],"counters":[{"version":1,"variation":0,"value":3E0,"count":10}]},"cat-food-amount":{"default":1E0,"contextKinds":["cat"],"counters":[{"version":1,"variation":0,"value":3E0,"count":1}]}}}
)");
    ASSERT_EQ(json, expected);
}
