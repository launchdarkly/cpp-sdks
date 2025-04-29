#include <gtest/gtest.h>

#include "events/event_factory.hpp"
#include "prereq_event_recorder/prereq_event_recorder.hpp"

#include <launchdarkly/context_builder.hpp>

using namespace launchdarkly;
using namespace launchdarkly::server_side;
using namespace launchdarkly::events;

TEST(PrereqEventRecorderTest, EmptyByDefault) {
    PrereqEventRecorder recorder{"foo"};
    ASSERT_TRUE(recorder.Prerequisites().empty());

    std::vector<std::string> prereqs = std::move(recorder).TakePrerequisites();
    ASSERT_TRUE(prereqs.empty());
}

TEST(PrereqEventRecorderTest, RecordsPrerequisites) {
    std::string const flag = "toplevel";

    PrereqEventRecorder recorder{flag};

    auto factory = EventFactory::WithoutReasons();

    auto const context = ContextBuilder().Kind("cat", "shadow").Build();

    recorder.SendAsync(factory.Eval("prereq1", context, std::nullopt,
                                    EvaluationReason::Fallthrough(false), false,
                                    flag));

    recorder.SendAsync(factory.Eval("prereq2", context, std::nullopt,
                                    EvaluationReason::Fallthrough(false), false,
                                    flag));

    auto const expectedPrereqs = std::vector<std::string>{"prereq1", "prereq2"};
    ASSERT_EQ(recorder.Prerequisites(), expectedPrereqs);
}

TEST(PrereqEventRecorderTest, IgnoresIrrelevantEvents) {
    PrereqEventRecorder recorder{"foo"};

    auto factory = EventFactory::WithoutReasons();

    auto const context = ContextBuilder().Kind("cat", "shadow").Build();

    recorder.SendAsync(factory.Identify(context));
    recorder.SendAsync(factory.UnknownFlag(
        "flag", context, EvaluationReason::Fallthrough(false), true));
    recorder.SendAsync(factory.Custom(context, "event", std::nullopt, 1.0));

    ASSERT_TRUE(recorder.Prerequisites().empty());
}

TEST(PrereqEventRecorderTest, IgnoresEvalEventsWithoutPrereqOf) {
    PrereqEventRecorder recorder{"toplevel"};

    auto factory = EventFactory::WithoutReasons();

    auto const context = ContextBuilder().Kind("cat", "shadow").Build();

    // Receiving an eval event without a prereq_of field shouldn't actually
    // happen when calling AllFlags, but regardless we should ignore it because
    // that would signify that it isn't a prerequisite.
    recorder.SendAsync(factory.Eval("not-a-prereq", context, std::nullopt,
                                    EvaluationReason::Fallthrough(false), false,
                                    std::nullopt));

    ASSERT_TRUE(recorder.Prerequisites().empty());
}

TEST(PrereqEventRecorderTest, TakesPrerequisites) {
    std::string const flag = "toplevel";

    PrereqEventRecorder recorder{flag};

    auto factory = EventFactory::WithoutReasons();

    auto const context = ContextBuilder().Kind("cat", "shadow").Build();

    recorder.SendAsync(factory.Eval("prereq1", context, std::nullopt,
                                    EvaluationReason::Fallthrough(false), false,
                                    flag));

    recorder.SendAsync(factory.Eval("prereq2", context, std::nullopt,
                                    EvaluationReason::Fallthrough(false), false,
                                    flag));

    auto const expectedPrereqs = std::vector<std::string>{"prereq1", "prereq2"};
    auto gotPrereqs = std::move(recorder).TakePrerequisites();
    ASSERT_EQ(gotPrereqs, expectedPrereqs);
    ASSERT_TRUE(recorder.Prerequisites().empty());
}
