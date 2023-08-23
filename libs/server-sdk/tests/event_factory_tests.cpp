#include <gtest/gtest.h>

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/data/evaluation_reason.hpp>
#include <launchdarkly/data_model/flag.hpp>

#include "events/event_factory.hpp"

using namespace launchdarkly;
using namespace launchdarkly::server_side;

class EventFactoryTests : public testing::Test {
   public:
    EventFactoryTests()
        : context_(ContextBuilder().Kind("cat", "shadow").Build()) {}
    Context context_;
};

TEST_F(EventFactoryTests, IncludesReasonIfInExperiment) {
    auto factory = EventFactory::WithoutReasons();
    auto event =
        factory.Eval("flag", context_, data_model::Flag{},
                     EvaluationReason::Fallthrough(true), false, std::nullopt);
    ASSERT_TRUE(std::get<events::FeatureEventParams>(event).reason.has_value());
}

TEST_F(EventFactoryTests, DoesNotIncludeReasonIfNotInExperiment) {
    auto factory = EventFactory::WithoutReasons();
    auto event =
        factory.Eval("flag", context_, data_model::Flag{},
                     EvaluationReason::Fallthrough(false), false, std::nullopt);
    ASSERT_FALSE(
        std::get<events::FeatureEventParams>(event).reason.has_value());
}

TEST_F(EventFactoryTests, IncludesReasonIfForcedByFactory) {
    auto factory = EventFactory::WithReasons();
    auto event =
        factory.Eval("flag", context_, data_model::Flag{},
                     EvaluationReason::Fallthrough(false), false, std::nullopt);
    ASSERT_TRUE(std::get<events::FeatureEventParams>(event).reason.has_value());
}
