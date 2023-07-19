#include <gtest/gtest.h>

#include "evaluation/detail/evaluation_stack.hpp"

using namespace launchdarkly::server_side::evaluation::detail;

struct TestString {
    static const inline std::string foo = "foo";
    static const inline std::string bar = "bar";
};

TEST(EvalStackTests, SegmentIsNoticed) {
    EvaluationStack stack;
    auto g1 = stack.NoticeSegment(TestString::foo);
    ASSERT_TRUE(g1);
    ASSERT_FALSE(stack.NoticeSegment(TestString::foo));
}

TEST(EvalStackTests, PrereqIsNoticed) {
    EvaluationStack stack;
    auto g1 = stack.NoticePrerequisite(TestString::foo);
    ASSERT_TRUE(g1);
    ASSERT_FALSE(stack.NoticePrerequisite(TestString::foo));
}

TEST(EvalStackTests, NestedScopes) {
    EvaluationStack stack;
    {
        auto g1 = stack.NoticeSegment(TestString::foo);
        ASSERT_TRUE(g1);
        ASSERT_FALSE(stack.NoticeSegment(TestString::foo));
        {
            auto g2 = stack.NoticeSegment(TestString::bar);
            ASSERT_TRUE(g2);
            ASSERT_FALSE(stack.NoticeSegment(TestString::bar));
            ASSERT_FALSE(stack.NoticeSegment(TestString::foo));
        }
        ASSERT_TRUE(stack.NoticeSegment(TestString::bar));
    }
    ASSERT_TRUE(stack.NoticeSegment(TestString::foo));
    ASSERT_TRUE(stack.NoticeSegment(TestString::bar));
}

TEST(EvalStackTests, SegmentAndPrereqHaveSeparateCaches) {
    EvaluationStack stack;
    auto g1 = stack.NoticeSegment(TestString::foo);
    ASSERT_TRUE(g1);
    auto g2 = stack.NoticePrerequisite(TestString::foo);
    ASSERT_TRUE(g2);
}

TEST(EvalStackTests, ImmediateDestructionOfGuard) {
    EvaluationStack stack;

    ASSERT_TRUE(stack.NoticeSegment(TestString::foo));
    ASSERT_TRUE(stack.NoticeSegment(TestString::foo));
    ASSERT_TRUE(stack.NoticeSegment(TestString::foo));
}
