#include <gtest/gtest.h>

#include "evaluation/operators.hpp"

using namespace launchdarkly::server_side::evaluation::operators;
using namespace launchdarkly::data_model;

TEST(OpTests, StartsWith) {
    EXPECT_TRUE(Match(Clause::Op::kStartsWith, "", ""));
    EXPECT_TRUE(Match(Clause::Op::kStartsWith, "a", ""));
    EXPECT_TRUE(Match(Clause::Op::kStartsWith, "a", "a"));

    EXPECT_TRUE(Match(Clause::Op::kStartsWith, "food", "foo"));
    EXPECT_FALSE(Match(Clause::Op::kStartsWith, "foo", "food"));

    EXPECT_FALSE(Match(Clause::Op::kStartsWith, "Food", "foo"));
}

TEST(OpTests, EndsWith) {
    EXPECT_TRUE(Match(Clause::Op::kEndsWith, "", ""));
    EXPECT_TRUE(Match(Clause::Op::kEndsWith, "a", ""));
    EXPECT_TRUE(Match(Clause::Op::kEndsWith, "a", "a"));

    EXPECT_TRUE(Match(Clause::Op::kEndsWith, "food", "ood"));
    EXPECT_FALSE(Match(Clause::Op::kEndsWith, "ood", "food"));

    EXPECT_FALSE(Match(Clause::Op::kEndsWith, "FOOD", "ood"));
}

TEST(OpTests, NumericComparisons) {
    EXPECT_TRUE(Match(Clause::Op::kLessThan, 0, 1));
    EXPECT_FALSE(Match(Clause::Op::kLessThan, 1, 0));
    EXPECT_FALSE(Match(Clause::Op::kLessThan, 0, 0));

    EXPECT_TRUE(Match(Clause::Op::kGreaterThan, 1, 0));
    EXPECT_FALSE(Match(Clause::Op::kGreaterThan, 0, 1));
    EXPECT_FALSE(Match(Clause::Op::kGreaterThan, 0, 0));

    EXPECT_TRUE(Match(Clause::Op::kLessThanOrEqual, 0, 1));
    EXPECT_TRUE(Match(Clause::Op::kLessThanOrEqual, 0, 0));
    EXPECT_FALSE(Match(Clause::Op::kLessThanOrEqual, 1, 0));

    EXPECT_TRUE(Match(Clause::Op::kGreaterThanOrEqual, 1, 0));
    EXPECT_TRUE(Match(Clause::Op::kGreaterThanOrEqual, 0, 0));
    EXPECT_FALSE(Match(Clause::Op::kGreaterThanOrEqual, 0, 1));
}

TEST(OpTests, DateComparisons) {
    EXPECT_TRUE(Match(Clause::Op::kBefore, "1985-04-12T23:20:50.52Z",
                      "1985-04-12T23:20:50.53Z"));
    EXPECT_TRUE(Match(Clause::Op::kAfter, "1985-04-12T23:20:50.53Z",
                      "1985-04-12T23:20:50.52Z"));
    EXPECT_TRUE(Match(Clause::Op::kBefore, "1985-04-12T23:20:50.52Z",
                      "2023-04-12T23:20:50.52Z"));
    EXPECT_TRUE(Match(Clause::Op::kAfter, "2023-04-12T23:20:50.52Z",
                      "1985-04-12T23:20:50.52Z"));
}
// Table-driven gtest that allows for specification of an input string
// and a regex expression. There is also a boolean to indicate if the match
// should succeed or not. The test then calls Match(Clause::Op::kMatches, ...)
// and verifies that the value matches or not based on the boolean.

struct RegexTest {
    std::string input;
    std::string regex;
    bool shouldMatch;
};

#define MATCH true
#define NO_MATCH false

class RegexTests : public ::testing::TestWithParam<RegexTest> {};

TEST_P(RegexTests, Matches) {
    auto const& param = GetParam();
    auto const result = Match(Clause::Op::kMatches, param.input, param.regex);

    EXPECT_EQ(result, param.shouldMatch)
        << "input: (" << (param.input.empty() ? "empty string" : param.input)
        << ")\nregex: (" << (param.regex.empty() ? "empty string" : param.regex)
        << ")";
}

INSTANTIATE_TEST_SUITE_P(RegexComparisons,
                         RegexTests,
                         ::testing::ValuesIn({
                             RegexTest{"", "", MATCH},
                             RegexTest{"a", "", MATCH},
                             RegexTest{"a", "a", MATCH},
                             RegexTest{"a", ".", MATCH},
                             RegexTest{"hello world", "hello.*rld", MATCH},
                             RegexTest{"hello world", "hello.*orl", MATCH},
                             RegexTest{"hello world", "l+", MATCH},
                             RegexTest{"hello world", "(world|planet)", MATCH},

                             RegexTest{"", ".", NO_MATCH},
                             RegexTest{"", R"(\)", NO_MATCH},
                             RegexTest{"hello world", "aloha", NO_MATCH},
                             RegexTest{"hello world", "***bad regex", NO_MATCH},
                         }));

#undef MATCH
#undef NO_MATCH
