#include <gtest/gtest.h>

#include "evaluation/evaluation_stack.hpp"

using namespace launchdarkly::server_side::evaluation;

TEST(EvalStackTests, SegmentIsNoticed) {
    EvaluationStack stack;
    auto g1 = stack.NoticeSegment("foo");
    ASSERT_TRUE(g1);
    ASSERT_FALSE(stack.NoticeSegment("foo"));
}

TEST(EvalStackTests, PrereqIsNoticed) {
    EvaluationStack stack;
    auto g1 = stack.NoticePrerequisite("foo");
    ASSERT_TRUE(g1);
    ASSERT_FALSE(stack.NoticePrerequisite("foo"));
}

TEST(EvalStackTests, NestedScopes) {
    EvaluationStack stack;
    {
        auto g1 = stack.NoticeSegment("foo");
        ASSERT_TRUE(g1);
        ASSERT_FALSE(stack.NoticeSegment("foo"));
        {
            auto g2 = stack.NoticeSegment("bar");
            ASSERT_TRUE(g2);
            ASSERT_FALSE(stack.NoticeSegment("bar"));
            ASSERT_FALSE(stack.NoticeSegment("foo"));
        }
        ASSERT_TRUE(stack.NoticeSegment("bar"));
    }
    ASSERT_TRUE(stack.NoticeSegment("foo"));
    ASSERT_TRUE(stack.NoticeSegment("bar"));
}

TEST(EvalStackTests, SegmentAndPrereqHaveSeparateCaches) {
    EvaluationStack stack;
    auto g1 = stack.NoticeSegment("foo");
    ASSERT_TRUE(g1);
    auto g2 = stack.NoticePrerequisite("foo");
    ASSERT_TRUE(g2);
}

TEST(EvalStackTests, ImmediateDestructionOfGuard) {
    EvaluationStack stack;

    ASSERT_TRUE(stack.NoticeSegment("foo"));
    ASSERT_TRUE(stack.NoticeSegment("foo"));
    ASSERT_TRUE(stack.NoticeSegment("foo"));
}
