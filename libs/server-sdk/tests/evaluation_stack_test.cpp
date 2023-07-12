#include <gtest/gtest.h>

#include "evaluation/detail/evaluation_stack.hpp"

using namespace launchdarkly::server_side::evaluation::detail;

TEST(EvalStackTests, SegmentIsNoticed) {
    EvaluationStack stack;
    auto g1 = stack.NoticeSegment("foo");
    ASSERT_TRUE(g1);
    auto g2 = stack.NoticeSegment("foo");
    ASSERT_FALSE(g2);
}

TEST(EvalStackTests, PrereqIsNoticed) {
    EvaluationStack stack;
    auto g1 = stack.NoticePrerequisite("foo");
    ASSERT_TRUE(g1);
    auto g2 = stack.NoticePrerequisite("foo");
    ASSERT_FALSE(g2);
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
    ASSERT_TRUE(stack.NoticeSegment("foo"));
    ASSERT_TRUE(stack.NoticePrerequisite("foo"));
}

TEST(EvalStackTests, ManyNoticedKeysAreForgotten) {
    EvaluationStack stack;

    for (std::size_t i = 0; i < 100; i++) {
        ASSERT_TRUE(stack.NoticeSegment(std::to_string(i)));
    }

    for (std::size_t i = 0; i < 100; i++) {
        ASSERT_TRUE(stack.NoticeSegment(std::to_string(i)));
    }
}
