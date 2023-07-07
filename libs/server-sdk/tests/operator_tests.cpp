#include <gtest/gtest.h>

#include <launchdarkly/context_builder.hpp>

#include "evaluation/evaluator.hpp"
#include "evaluation/operators.hpp"
#include "evaluation/rules.hpp"

using namespace launchdarkly::server_side::evaluation::operators;
using namespace launchdarkly::data_model;
using namespace launchdarkly;

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

// We can only support microsecond precision due to resolution of the
// system_clock::time_point.
//
// The spec says we should support no more than 9 digits
// (nanoseconds.) This test attempts to verify that microsecond precision
// differences are handled.
TEST(OpTests, DateComparisonMicrosecondPrecision) {
    auto dates = std::vector<std::pair<std::string, std::string>>{
        // Using Zulu suffix.
        {"2023-10-08T02:00:00.000001Z", "2023-10-08T02:00:00.000002Z"},
        // Using offset suffix.
        {"2023-10-08T02:00:00.000001+00:00",
         "2023-10-08T02:00:00.000002+00:00"}};

    for (auto const& [date1, date2] : dates) {
        EXPECT_TRUE(Match(Clause::Op::kBefore, date1, date2))
            << date1 << " < " << date2;

        EXPECT_FALSE(Match(Clause::Op::kAfter, date1, date2))
            << date1 << " not > " << date2;

        EXPECT_FALSE(Match(Clause::Op::kBefore, date2, date1))
            << date2 << " not < " << date1;

        EXPECT_TRUE(Match(Clause::Op::kAfter, date2, date1))
            << date2 << " > " << date1;
    }
}

// Comparison should be effectively "equal" for all combinations.
TEST(OpTests, DateComparisonFailsWithMoreThanMicrosecondPrecision) {
    auto dates = std::vector<std::pair<std::string, std::string>>{
        // Using Zulu suffix.
        {"2023-10-08T02:00:00.000001Z", "2023-10-08T02:00:00.0000011Z"},
        // Using offset suffix.
        {"2023-10-08T02:00:00.0000000001+00:00",
         "2023-10-08T02:00:00.00000000011+00:00"}};

    //
    //    EXPECT_FALSE(Match(Clause::Op::kBefore, kDate1, kDate2))
    //        << kDate1 << " < " << kDate2;
    //
    //    EXPECT_FALSE(Match(Clause::Op::kAfter, kDate1, kDate2))
    //        << kDate1 << " not > " << kDate2;
    //
    //    EXPECT_FALSE(Match(Clause::Op::kBefore, kDate2, kDate1))
    //        << kDate2 << " not < " << kDate1;
    //
    //    EXPECT_FALSE(Match(Clause::Op::kAfter, kDate2, kDate1))
    //        << kDate2 << " > " << kDate1;

    for (auto const& [date1, date2] : dates) {
        EXPECT_FALSE(Match(Clause::Op::kBefore, date1, date2))
            << date1 << " < " << date2;

        EXPECT_FALSE(Match(Clause::Op::kAfter, date1, date2))
            << date1 << " not > " << date2;

        EXPECT_FALSE(Match(Clause::Op::kBefore, date2, date1))
            << date2 << " not < " << date1;

        EXPECT_FALSE(Match(Clause::Op::kAfter, date2, date1))
            << date2 << " > " << date1;
    }
    // Somehow date parsing is succeeding when we have the offset.
}

// Because RFC3339 timestamps may use 'Z' to indicate a 00:00 offset,
// we should ensure these timestamps can be compared to timestamps using normal
// offsets.
TEST(OpTests, AcceptsZuluAndNormalTimezoneOffsets) {
    const std::string kDate1 = "1985-04-12T23:20:50Z";
    const std::string kDate2 = "1986-04-12T23:20:50-01:00";

    EXPECT_TRUE(Match(Clause::Op::kBefore, kDate1, kDate2));
    EXPECT_FALSE(Match(Clause::Op::kAfter, kDate1, kDate2));

    EXPECT_FALSE(Match(Clause::Op::kBefore, kDate2, kDate1));
    EXPECT_TRUE(Match(Clause::Op::kAfter, kDate2, kDate1));
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

class RegexTests : public ::testing::TestWithParam<RegexTest> {};

TEST_P(RegexTests, Matches) {
    auto const& param = GetParam();
    auto const result = Match(Clause::Op::kMatches, param.input, param.regex);

    EXPECT_EQ(result, param.shouldMatch)
        << "input: (" << (param.input.empty() ? "empty string" : param.input)
        << ")\nregex: (" << (param.regex.empty() ? "empty string" : param.regex)
        << ")";
}

#define MATCH true
#define NO_MATCH false

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
    bool swapped = false;

    if (param.op == SEMVER_EQUAL) {
        result = Match(Clause::Op::kSemVerEqual, param.lhs, param.rhs);
        swapped = Match(Clause::Op::kSemVerEqual, param.rhs, param.lhs);
    } else if (param.op == SEMVER_NOT_EQUAL) {
        result = !Match(Clause::Op::kSemVerEqual, param.lhs, param.rhs);
        swapped = !Match(Clause::Op::kSemVerEqual, param.rhs, param.lhs);
    } else if (param.op == SEMVER_GREATER) {
        result = Match(Clause::Op::kSemVerGreaterThan, param.lhs, param.rhs);
        swapped = Match(Clause::Op::kSemVerLessThan, param.rhs, param.lhs);
    } else if (param.op == SEMVER_LESS) {
        result = Match(Clause::Op::kSemVerLessThan, param.lhs, param.rhs);
        swapped = Match(Clause::Op::kSemVerGreaterThan, param.rhs, param.lhs);
    } else {
        FAIL() << "Invalid operator: " << param.op;
    }

    EXPECT_EQ(result, param.shouldMatch)
        << param.lhs << " " << param.op << " " << param.rhs << " should be "
        << (param.shouldMatch ? "true" : "false");

    EXPECT_EQ(result, swapped)
        << "commutative property invalid for " << param.lhs << " " << param.op
        << " " << param.rhs;
}

INSTANTIATE_TEST_SUITE_P(
    SemVerComparisons,
    SemVerTests,
    ::testing::ValuesIn(
        {SemVerTest{"2.0.0", "2.0.0", SEMVER_EQUAL, MATCH},
         SemVerTest{"2.0", "2.0.0", SEMVER_EQUAL, MATCH},
         SemVerTest{"2", "2.0.0", SEMVER_EQUAL, MATCH},
         SemVerTest{"2", "2.0.0+123", SEMVER_EQUAL, MATCH},
         SemVerTest{"2+456", "2.0.0+123", SEMVER_EQUAL, MATCH},
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
