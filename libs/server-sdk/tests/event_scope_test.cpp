#include <gtest/gtest.h>

#include <launchdarkly/context_builder.hpp>

#include "spy_event_processor.hpp"

#include "events/event_scope.hpp"

using namespace launchdarkly;
using namespace launchdarkly::server_side;

TEST(EventScope, DefaultConstructedScopeHasNoObservableEffects) {
    EventScope default_scope;
    default_scope.Send([](EventFactory const& factory) {
        return factory.Identify(ContextBuilder().Kind("cat", "shadow").Build());
    });
}

TEST(EventScope, SendWithNullProcessorHasNoObservableEffects) {
    EventScope scope(nullptr, EventFactory::WithoutReasons());
    scope.Send([](EventFactory const& factory) {
        return factory.Identify(ContextBuilder().Kind("cat", "shadow").Build());
    });
}

TEST(EventScope, ForwardsEvents) {
    SpyEventProcessor processor;
    EventScope scope(&processor, EventFactory::WithoutReasons());

    const std::size_t kEventCount = 10;

    for (std::size_t i = 0; i < kEventCount; ++i) {
        scope.Send([](EventFactory const& factory) {
            return factory.Identify(
                ContextBuilder().Kind("cat", "shadow").Build());
        });
    }

    ASSERT_TRUE(processor.Count(kEventCount));
}

TEST(EventScope, ForwardsCorrectEventTypes) {
    SpyEventProcessor processor;
    EventScope scope(&processor, EventFactory::WithoutReasons());

    scope.Send([](EventFactory const& factory) {
        return factory.Identify(ContextBuilder().Kind("cat", "shadow").Build());
    });

    scope.Send([](EventFactory const& factory) {
        return factory.UnknownFlag(
            "flag", ContextBuilder().Kind("cat", "shadow").Build(),
            EvaluationReason::Fallthrough(false), true);
    });

    scope.Send([](EventFactory const& factory) {
        return factory.Eval("flag",
                            ContextBuilder().Kind("cat", "shadow").Build(),
                            std::nullopt, EvaluationReason::Fallthrough(false),
                            false, std::nullopt);
    });

    scope.Send([](EventFactory const& factory) {
        return factory.Custom(ContextBuilder().Kind("cat", "shadow").Build(),
                              "event", std::nullopt, std::nullopt);
    });

    ASSERT_TRUE(processor.Count(4));
    ASSERT_TRUE(processor.Kind<events::IdentifyEventParams>(0));
    ASSERT_TRUE(processor.Kind<events::FeatureEventParams>(1));
    ASSERT_TRUE(processor.Kind<events::FeatureEventParams>(2));
    ASSERT_TRUE(processor.Kind<events::TrackEventParams>(3));
}
