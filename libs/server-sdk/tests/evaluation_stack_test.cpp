#include <gtest/gtest.h>

#include "evaluation/detail/evaluation_stack.hpp"

using namespace launchdarkly::server_side::evaluation::detail;

TEST(EvalStackTests, NoticeSegments) {
    EvaluationStack stack;
    ASSERT_FALSE(stack.SeenSegment("foo"));
    {
        auto guard = stack.NoticeSegment("foo");
        ASSERT_TRUE(stack.SeenSegment("foo"));
    }
    ASSERT_FALSE(stack.SeenSegment("foo"));
}

TEST(EvalStackTests, NoticePrerequisites) {
    EvaluationStack stack;
    ASSERT_FALSE(stack.SeenPrerequisite("foo"));
    {
        auto guard = stack.NoticePrerequisite("foo");
        ASSERT_TRUE(stack.SeenPrerequisite("foo"));
    }
    ASSERT_FALSE(stack.SeenPrerequisite("foo"));
}

TEST(EvalStackTests, SegmentAndPrereqHaveSeparateCaches) {
    EvaluationStack stack;
    auto g1 = stack.NoticeSegment("foo");
    ASSERT_FALSE(stack.SeenPrerequisite("foo"));
    auto g2 = stack.NoticePrerequisite("bar");
    ASSERT_FALSE(stack.SeenSegment("bar"));
}

TEST(EvalStackTests, ManyNoticedKeysAreForgotten) {
    EvaluationStack stack;

    for (std::size_t i = 0; i < 100; i++) {
        auto guard = stack.NoticeSegment(std::to_string(i));
        ASSERT_TRUE(stack.SeenSegment(std::to_string(i)));
    }

    for (std::size_t i = 0; i < 100; i++) {
        ASSERT_FALSE(stack.SeenSegment(std::to_string(i)));
    }
}
