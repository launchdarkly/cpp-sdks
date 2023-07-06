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

TEST(OpTests, DateComparisonMicrosecondPrecision) {
    EXPECT_TRUE(Match(Clause::Op::kBefore, "2021-10-08T02:00:00.000001-00:00",
                      "2022-10-08T02:00:00.000002-00:00"));

    EXPECT_FALSE(Match(Clause::Op::kAfter, "2021-10-08T02:00:00.000001-00:00",
                       "2022-10-08T02:00:00.000002-00:00"));

    EXPECT_TRUE(Match(Clause::Op::kBefore, "2021-10-08T02:00:01.234567-00:00",
                      "2022-10-08T02:00:01.234568-00:00"));

    EXPECT_FALSE(Match(Clause::Op::kAfter, "2021-10-08T02:00:01.234567-00:00",
                       "2022-10-08T02:00:01.234568-00:00"));

    EXPECT_FALSE(Match(Clause::Op::kBefore, "2021-10-08T02:00:00.000001-00:00",
                       "2021-10-08T02:00:00.000001-00:00"));

    EXPECT_FALSE(Match(Clause::Op::kAfter, "2021-10-08T02:00:00.000001-00:00",
                       "2021-10-08T02:00:00.000001-00:00"));
}

TEST(OpTests, DateComparisonMicrosecodPrecisionOutOfBounds) {
    EXPECT_FALSE(Match(Clause::Op::kBefore, "2021-oo-08T02:00:00.000001-00:00",
                       "2021-10-08T02:00:00.0000011-00:00"));
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
TEST(OpTests, AcceptsZuluTimezone) {
    EXPECT_TRUE(Match(Clause::Op::kBefore, "1985-04-12T23:20:50Z",
                      "1985-04-12T23:20:51Z"));
}

TEST(OpTests, InvalidDates) {
    EXPECT_FALSE(Match(Clause::Op::kBefore, "2021-01-08T02:00:00-00:00",
                       "2021-12345-08T02:00:00-00:00"));

    EXPECT_FALSE(Match(Clause::Op::kAfter, "2021-12345-08T02:00:00-00:00",
                       "2021-01-08T02:00:00-00:00"));

    EXPECT_FALSE(Match(Clause::Op::kBefore, "foo", "bar"));

    EXPECT_FALSE(Match(Clause::Op::kAfter, "foo", "bar"));

    EXPECT_FALSE(Match(Clause::Op::kBefore, "", "bar"));
    EXPECT_FALSE(Match(Clause::Op::kAfter, "", "bar"));

    EXPECT_FALSE(Match(Clause::Op::kBefore, "foo", ""));
    EXPECT_FALSE(Match(Clause::Op::kAfter, "foo", ""));
}

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

#define SEMVER_NOT_EQUAL "!="
#define SEMVER_EQUAL "=="
#define SEMVER_GREATER ">"
#define SEMVER_LESS "<"

struct SemVerTest {
    std::string lhs;
    std::string rhs;
    std::string op;
    bool shouldMatch;
};

class SemVerTests : public ::testing::TestWithParam<SemVerTest> {};

TEST_P(SemVerTests, Matches) {
    auto const& param = GetParam();

    bool result = false;

    if (param.op == SEMVER_EQUAL) {
        result = Match(Clause::Op::kSemVerEqual, param.lhs, param.rhs);
    } else if (param.op == SEMVER_NOT_EQUAL) {
        result = !Match(Clause::Op::kSemVerEqual, param.lhs, param.rhs);
    } else if (param.op == SEMVER_GREATER) {
        result = Match(Clause::Op::kSemVerGreaterThan, param.lhs, param.rhs);
    } else if (param.op == SEMVER_LESS) {
        result = Match(Clause::Op::kSemVerLessThan, param.lhs, param.rhs);
    } else {
        FAIL() << "Invalid operator: " << param.op;
    }

    EXPECT_EQ(result, param.shouldMatch)
        << param.lhs << " " << param.op << " " << param.rhs << " should be "
        << (param.shouldMatch ? "true" : "false");
}

INSTANTIATE_TEST_SUITE_P(
    SemVerComparisons,
    SemVerTests,
    ::testing::ValuesIn(
        {SemVerTest{"2.0.0", "2.0.0", SEMVER_EQUAL, MATCH},
         SemVerTest{"2.0", "2.0.0", SEMVER_EQUAL, MATCH},
         SemVerTest{"2", "2.0.0", SEMVER_EQUAL, MATCH},
         SemVerTest{"2.0.0", "3.0.0", SEMVER_NOT_EQUAL, MATCH},
         SemVerTest{"2.0.0", "2.1.0", SEMVER_NOT_EQUAL, MATCH},
         SemVerTest{"2.0.0", "2.0.1", SEMVER_NOT_EQUAL, MATCH},
         SemVerTest{"3.0.0", "2.0.0", SEMVER_GREATER, MATCH},
         SemVerTest{"2.1.0", "2.0.0", SEMVER_GREATER, MATCH},
         SemVerTest{"2.0.1", "2.0.0", SEMVER_GREATER, MATCH},
         SemVerTest{"2.0.0", "2.0.0", SEMVER_GREATER, NO_MATCH},
         SemVerTest{"1.9.0", "2.0.0", SEMVER_GREATER, NO_MATCH},
         SemVerTest{"2.0.0-rc", "2.0.0", SEMVER_GREATER, NO_MATCH},
         SemVerTest{"2.0.0+build", "2.0.0", SEMVER_GREATER, NO_MATCH},
         SemVerTest{"2.0.0+build", "2.0.0", SEMVER_EQUAL, MATCH},
         SemVerTest{"2.0.0", "200", SEMVER_EQUAL, NO_MATCH},
         SemVerTest{"2.0.0-rc.10.green", "2.0.0-rc.2.green", SEMVER_GREATER,
                    MATCH},
         SemVerTest{"2.0.0-rc.2.red", "2.0.0-rc.2.green", SEMVER_GREATER,
                    MATCH},
         SemVerTest{"2.0.0-rc.2.green.1", "2.0.0-rc.2.green", SEMVER_GREATER,
                    MATCH},
         SemVerTest{"2.0.0-rc.1.very.long.prerelease.version.1234567.keeps."
                    "going+123124",
                    "2.0.0", SEMVER_LESS, MATCH},
         SemVerTest{"1", "2", SEMVER_LESS, MATCH},
         SemVerTest{"0", "1", SEMVER_LESS, MATCH}}));

#undef SEMVER_NOT_EQUAL
#undef SEMVER_EQUAL
#undef SEMVER_GREATER
#undef SEMVER_LESS

#undef MATCH
#undef NO_MATCH
