#include <gtest/gtest.h>

#include <launchdarkly/context_builder.hpp>

#include "evaluation/evaluator.hpp"
#include "evaluation/rules.hpp"

#include "test_store.hpp"

using namespace launchdarkly::data_model;
using namespace launchdarkly::server_side;
using namespace launchdarkly;

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
    const static int DATE_MS_NEGATIVE;
    const static std::string INVALID_DATE;
};

const std::string AllOperatorsTest::DATE_STR1 = "2017-12-06T00:00:00.000-07:00";
const std::string AllOperatorsTest::DATE_STR2 = "2017-12-06T00:01:01.000-07:00";
int const AllOperatorsTest::DATE_MS1 = 10000000;
int const AllOperatorsTest::DATE_MS2 = 10000001;
int const AllOperatorsTest::DATE_MS_NEGATIVE = -10000;
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

    Clause clause{param.op, std::move(clauseValues), false, ContextKind("user"),
                  "attr"};

    auto context = launchdarkly::ContextBuilder()
                       .Kind("user", "key")
                       .Set("attr", param.contextValue)
                       .Build();
    ASSERT_TRUE(context.Valid());

    EvaluationStack stack;

    auto store = test_store::Empty();

    auto result = launchdarkly::server_side::evaluation::Match(clause, context,
                                                               *store, stack);
    ASSERT_EQ(result, param.expected)
        << context.Get("user", "attr") << " " << clause.op << " "
        << clause.values << " should be " << param.expected;
}

#define MATCH true
#define NO_MATCH false

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
        ClauseTest{Clause::Op::kBefore, AllOperatorsTest::DATE_MS_NEGATIVE,
                   AllOperatorsTest::DATE_MS1, NO_MATCH},
        ClauseTest{Clause::Op::kAfter, AllOperatorsTest::DATE_MS1,
                   AllOperatorsTest::DATE_MS_NEGATIVE, NO_MATCH},

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

#undef MATCH
#undef NO_MATCH
