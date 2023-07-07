#include <gtest/gtest.h>

#include <launchdarkly/context_builder.hpp>

#include "evaluation/evaluator.hpp"
#include "evaluation/operators.hpp"
#include "evaluation/rules.hpp"

#include "flag_manager/flag_store.hpp"

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

struct ClauseTest {
    Clause::Op op;
    launchdarkly::Value contextValue;
    launchdarkly::Value clauseValue;
    bool expected;
};

class AllOperatorsTest : public ::testing::TestWithParam<ClauseTest> {
   public:
    const static std::string DATE_STR1;
    const static std::string DATE_STR2;
    const static int DATE_MS1;
    const static int DATE_MS2;
    const static std::string INVALID_DATE;
};

const std::string AllOperatorsTest::DATE_STR1 = "2017-12-06T00:00:00.000-07:00";
const std::string AllOperatorsTest::DATE_STR2 = "2017-12-06T00:01:01.000-07:00";
int const AllOperatorsTest::DATE_MS1 = 10000000;
int const AllOperatorsTest::DATE_MS2 = 10000001;
const std::string AllOperatorsTest::INVALID_DATE = "hey what's this?";

TEST_P(AllOperatorsTest, Matches) {
    using namespace launchdarkly::server_side::evaluation::detail;
    using namespace launchdarkly;

    auto const& param = GetParam();

    std::vector<Value> clauseValues;

    if (param.clauseValue.IsArray()) {
        auto const& as_array = param.clauseValue.AsArray();
        clauseValues = std::vector<Value>{as_array.begin(), as_array.end()};
    } else {
        clauseValues.push_back(param.clauseValue);
    }

    Clause clause{param.op, std::move(clauseValues), false, "user", "attr"};

    auto context = launchdarkly::ContextBuilder()
                       .Kind("user", "key")
                       .Set("attr", param.contextValue)
                       .Build();
    ASSERT_TRUE(context.Valid());

    EvaluationStack stack{20};
    server_side::flag_manager::FlagStore store;

    auto result = launchdarkly::server_side::evaluation::Match(clause, context,
                                                               store, stack);
    ASSERT_EQ(result, param.expected)
        << context.Get("user", "attr") << " " << clause.op << " "
        << clause.values << " should be " << param.expected;
}

INSTANTIATE_TEST_SUITE_P(
    NumericClauses,
    AllOperatorsTest,
    ::testing::ValuesIn({
        ClauseTest{Clause::Op::kIn, 99, 99, MATCH},
        ClauseTest{Clause::Op::kIn, 99.0, 99, MATCH},
        ClauseTest{Clause::Op::kIn, 99, 99.0, MATCH},
        ClauseTest{Clause::Op::kIn, 99,
                   std::vector<launchdarkly::Value>{99, 98, 97, 96}, MATCH},
        ClauseTest{Clause::Op::kIn, 99.0001, 99.0001, MATCH},
        ClauseTest{Clause::Op::kIn, 99.0001,
                   std::vector<launchdarkly::Value>{99.0001, 98.0, 97.0, 96.0},
                   MATCH},
        ClauseTest{Clause::Op::kLessThan, 1, 1.99999, MATCH},
        ClauseTest{Clause::Op::kLessThan, 1.99999, 1, NO_MATCH},
        ClauseTest{Clause::Op::kLessThan, 1, 2, MATCH},
        ClauseTest{Clause::Op::kLessThanOrEqual, 1, 1.0, MATCH},
        ClauseTest{Clause::Op::kGreaterThan, 2, 1.99999, MATCH},
        ClauseTest{Clause::Op::kGreaterThan, 1.99999, 2, NO_MATCH},
        ClauseTest{Clause::Op::kGreaterThan, 2, 1, MATCH},
        ClauseTest{Clause::Op::kGreaterThanOrEqual, 1, 1.0, MATCH},

    }));

INSTANTIATE_TEST_SUITE_P(
    StringClauses,
    AllOperatorsTest,
    ::testing::ValuesIn({
        ClauseTest{Clause::Op::kIn, "x", "x", MATCH},
        ClauseTest{Clause::Op::kIn, "x",
                   std::vector<launchdarkly::Value>{"x", "a", "b", "c"}, MATCH},
        ClauseTest{Clause::Op::kIn, "x", "xyz", NO_MATCH},
        ClauseTest{Clause::Op::kStartsWith, "xyz", "x", MATCH},
        ClauseTest{Clause::Op::kStartsWith, "x", "xyz", NO_MATCH},
        ClauseTest{Clause::Op::kEndsWith, "xyz", "z", MATCH},
        ClauseTest{Clause::Op::kEndsWith, "z", "xyz", NO_MATCH},
        ClauseTest{Clause::Op::kContains, "xyz", "y", MATCH},
        ClauseTest{Clause::Op::kContains, "y", "xyz", NO_MATCH},
    }));

INSTANTIATE_TEST_SUITE_P(
    MixedStringAndNumbers,
    AllOperatorsTest,
    ::testing::ValuesIn({
        ClauseTest{Clause::Op::kIn, "99", 99, NO_MATCH},
        ClauseTest{Clause::Op::kIn, 99, "99", NO_MATCH},
        ClauseTest{Clause::Op::kContains, "99", 99, NO_MATCH},
        ClauseTest{Clause::Op::kStartsWith, "99", 99, NO_MATCH},
        ClauseTest{Clause::Op::kEndsWith, "99", 99, NO_MATCH},
        ClauseTest{Clause::Op::kLessThanOrEqual, "99", 99, NO_MATCH},
        ClauseTest{Clause::Op::kLessThanOrEqual, 99, "99", NO_MATCH},
        ClauseTest{Clause::Op::kGreaterThanOrEqual, "99", 99, NO_MATCH},
        ClauseTest{Clause::Op::kGreaterThanOrEqual, 99, "99", NO_MATCH},
    }));

INSTANTIATE_TEST_SUITE_P(
    BooleanEquality,
    AllOperatorsTest,
    ::testing::ValuesIn({
        ClauseTest{Clause::Op::kIn, true, true, MATCH},
        ClauseTest{Clause::Op::kIn, false, false, MATCH},
        ClauseTest{Clause::Op::kIn, true, false, NO_MATCH},
        ClauseTest{Clause::Op::kIn, false, true, NO_MATCH},
        ClauseTest{Clause::Op::kIn, true,
                   std::vector<launchdarkly::Value>{false, true}, MATCH},
    }));

INSTANTIATE_TEST_SUITE_P(
    ArrayEquality,
    AllOperatorsTest,
    ::testing::ValuesIn({
        ClauseTest{Clause::Op::kIn, {{"x"}}, {{"x"}}, MATCH},
        ClauseTest{Clause::Op::kIn, {{"x"}}, {"x"}, NO_MATCH},
        ClauseTest{Clause::Op::kIn, {{"x"}}, {{"x"}, {"a"}, {"b"}}, MATCH},
    }));

INSTANTIATE_TEST_SUITE_P(
    ObjectEquality,
    AllOperatorsTest,
    ::testing::ValuesIn({
        ClauseTest{Clause::Op::kIn, Value::Object({{"x", "1"}}),
                   Value::Object({{"x", "1"}}), MATCH},
        ClauseTest{Clause::Op::kIn, Value::Object({{"x", "1"}}),
                   std::vector<launchdarkly::Value>{
                       Value::Object({{"x", "1"}}),
                       Value::Object({{"a", "2"}}),
                       Value::Object({{"b", "3"}}),
                   },
                   MATCH},
    }));

INSTANTIATE_TEST_SUITE_P(
    RegexMatch,
    AllOperatorsTest,
    ::testing::ValuesIn({
        ClauseTest{Clause::Op::kMatches, "hello world", "hello.*rld", MATCH},
        ClauseTest{Clause::Op::kMatches, "hello world", "hello.*orl", MATCH},
        ClauseTest{Clause::Op::kMatches, "hello world", "l+", MATCH},
        ClauseTest{Clause::Op::kMatches, "hello world", "(world|planet)",
                   MATCH},
        ClauseTest{Clause::Op::kMatches, "hello world", "aloha", NO_MATCH},
        ClauseTest{Clause::Op::kMatches, "hello world", "***bad regex",
                   NO_MATCH},
    }));

INSTANTIATE_TEST_SUITE_P(
    DateClauses,
    AllOperatorsTest,
    ::testing::ValuesIn({
        ClauseTest{Clause::Op::kBefore, AllOperatorsTest::DATE_STR1,
                   AllOperatorsTest::DATE_STR2, MATCH},
        ClauseTest{Clause::Op::kBefore, AllOperatorsTest::DATE_MS1,
                   AllOperatorsTest::DATE_MS2, MATCH},
        ClauseTest{Clause::Op::kBefore, AllOperatorsTest::DATE_STR2,
                   AllOperatorsTest::DATE_STR1, NO_MATCH},
        ClauseTest{Clause::Op::kBefore, AllOperatorsTest::DATE_MS2,
                   AllOperatorsTest::DATE_MS1, NO_MATCH},
        ClauseTest{Clause::Op::kBefore, AllOperatorsTest::DATE_STR1,
                   AllOperatorsTest::DATE_STR1, NO_MATCH},
        ClauseTest{Clause::Op::kBefore, AllOperatorsTest::DATE_MS1,
                   AllOperatorsTest::DATE_MS1, NO_MATCH},
        ClauseTest{Clause::Op::kBefore, Value::Null(),
                   AllOperatorsTest::DATE_STR1, NO_MATCH},
        ClauseTest{Clause::Op::kBefore, AllOperatorsTest::DATE_STR1,
                   AllOperatorsTest::INVALID_DATE, NO_MATCH},
        ClauseTest{Clause::Op::kAfter, AllOperatorsTest::DATE_STR2,
                   AllOperatorsTest::DATE_STR1, MATCH},
        ClauseTest{Clause::Op::kAfter, AllOperatorsTest::DATE_MS2,
                   AllOperatorsTest::DATE_MS1, MATCH},
        ClauseTest{Clause::Op::kAfter, AllOperatorsTest::DATE_STR1,
                   AllOperatorsTest::DATE_STR2, NO_MATCH},
        ClauseTest{Clause::Op::kAfter, AllOperatorsTest::DATE_MS1,
                   AllOperatorsTest::DATE_MS2, NO_MATCH},
        ClauseTest{Clause::Op::kAfter, AllOperatorsTest::DATE_STR1,
                   AllOperatorsTest::DATE_STR1, NO_MATCH},
        ClauseTest{Clause::Op::kAfter, AllOperatorsTest::DATE_MS1,
                   AllOperatorsTest::DATE_MS1, NO_MATCH},
        ClauseTest{Clause::Op::kAfter, Value::Null(),
                   AllOperatorsTest::DATE_STR1, NO_MATCH},
        ClauseTest{Clause::Op::kAfter, AllOperatorsTest::DATE_STR1,
                   AllOperatorsTest::INVALID_DATE, NO_MATCH},
    }));

INSTANTIATE_TEST_SUITE_P(
    SemVerTests,
    AllOperatorsTest,
    ::testing::ValuesIn(
        {ClauseTest{Clause::Op::kSemVerEqual, "2.0.0", "2.0.0", MATCH},
         ClauseTest{Clause::Op::kSemVerEqual, "2.0", "2.0.0", MATCH},
         ClauseTest{Clause::Op::kSemVerEqual, "2-rc1", "2.0.0-rc1", MATCH},
         ClauseTest{Clause::Op::kSemVerEqual, "2+build2", "2.0.0+build2",
                    MATCH},
         ClauseTest{Clause::Op::kSemVerEqual, "2.0.0", "2.0.1", NO_MATCH},
         ClauseTest{Clause::Op::kSemVerLessThan, "2.0.0", "2.0.1", MATCH},
         ClauseTest{Clause::Op::kSemVerLessThan, "2.0", "2.0.1", MATCH},
         ClauseTest{Clause::Op::kSemVerLessThan, "2.0.1", "2.0.0", NO_MATCH},
         ClauseTest{Clause::Op::kSemVerLessThan, "2.0.1", "2.0", NO_MATCH},
         ClauseTest{Clause::Op::kSemVerLessThan, "2.0.1", "xbad%ver", NO_MATCH},
         ClauseTest{Clause::Op::kSemVerLessThan, "2.0.0-rc", "2.0.0-rc.beta",
                    MATCH},
         ClauseTest{Clause::Op::kSemVerGreaterThan, "2.0.1", "2.0", MATCH},
         ClauseTest{Clause::Op::kSemVerGreaterThan, "10.0.1", "2.0", MATCH},
         ClauseTest{Clause::Op::kSemVerGreaterThan, "2.0.0", "2.0.1", NO_MATCH},
         ClauseTest{Clause::Op::kSemVerGreaterThan, "2.0", "2.0.1", NO_MATCH},
         ClauseTest{Clause::Op::kSemVerGreaterThan, "2.0.1", "xbad%ver",
                    NO_MATCH},
         ClauseTest{Clause::Op::kSemVerGreaterThan, "2.0.0-rc.1", "2.0.0-rc.0",
                    MATCH}}));
