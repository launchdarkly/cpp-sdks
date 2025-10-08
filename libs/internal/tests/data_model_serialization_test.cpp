#include <gtest/gtest.h>

#include <boost/json/value_from.hpp>

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/detail/serialization/json_primitives.hpp>
#include <launchdarkly/serialization/json_rule_clause.hpp>
#include <launchdarkly/serialization/json_sdk_data_set.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

using namespace launchdarkly;
using launchdarkly::data_model::ContextKind;

TEST(SDKDataSetTests, DeserializesEmptyDataSet) {
    auto result =
        boost::json::value_to<tl::expected<data_model::SDKDataSet, JsonError>>(
            boost::json::parse("{}"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->segments.empty());
    ASSERT_TRUE(result->flags.empty());
}

TEST(SDKDataSetTests, ErrorOnInvalidSchema) {
    auto result =
        boost::json::value_to<tl::expected<data_model::SDKDataSet, JsonError>>(
            boost::json::parse("[]"));
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), JsonError::kSchemaFailure);
}

TEST(SDKDataSetTests, DeserializesZeroSegments) {
    auto result =
        boost::json::value_to<tl::expected<data_model::SDKDataSet, JsonError>>(
            boost::json::parse(R"({"segments":{}})"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->segments.empty());
}

TEST(SDKDataSetTests, DeserializesZeroFlags) {
    auto result =
        boost::json::value_to<tl::expected<data_model::SDKDataSet, JsonError>>(
            boost::json::parse(R"({"flags":{}})"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->flags.empty());
}

TEST(SegmentTests, DeserializesMinimumValid) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<data_model::Segment>, JsonError>>(
        boost::json::parse(R"({"key":"foo", "version": 42})"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());

    ASSERT_EQ(result.value()->version, 42);
    ASSERT_EQ(result.value()->key, "foo");
}

TEST(SegmentTests, TolerantOfUnrecognizedFields) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<data_model::Segment>, JsonError>>(
        boost::json::parse(
            R"({"key":"foo", "version": 42, "somethingRandom" : true})"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result.value());
}

TEST(SegmentRuleTests, DeserializesMinimumValid) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Rule, JsonError>>(boost::json::parse(
        R"({"clauses": [{"attribute": "", "op": "in", "values": ["a"]}]})"));
    ASSERT_TRUE(result);

    auto const& clauses = result->clauses;
    ASSERT_EQ(clauses.size(), 1);

    auto const& clause = clauses.at(0);
    ASSERT_EQ(clause.op, data_model::Clause::Op::kIn);
}

TEST(SegmentRuleTests, TolerantOfUnrecognizedFields) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Rule, JsonError>>(boost::json::parse(
        R"({"somethingRandom": true, "clauses": [{"attribute": "", "op": "in", "values": ["a"]}]})"));

    ASSERT_TRUE(result);
}

TEST(SegmentRuleTests, DeserializesSimpleAttributeReference) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Rule, JsonError>>(boost::json::parse(
        R"({"rolloutContextKind" : "foo", "bucketBy" : "bar", "clauses": []})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->rolloutContextKind, ContextKind("foo"));
    ASSERT_EQ(result->bucketBy, AttributeReference("bar"));
}

TEST(SegmentRuleTests, DeserializesPointerAttributeReference) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Rule, JsonError>>(boost::json::parse(
        R"({"rolloutContextKind" : "foo", "bucketBy" : "/foo/bar", "clauses": []})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->rolloutContextKind, ContextKind("foo"));
    ASSERT_EQ(result->bucketBy, AttributeReference("/foo/bar"));
}

TEST(SegmentRuleTests, DeserializesEscapedReference) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Rule, JsonError>>(boost::json::parse(
        R"({"rolloutContextKind" : "foo", "bucketBy" : "/~1foo~1bar", "clauses": []})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->rolloutContextKind, ContextKind("foo"));
    ASSERT_EQ(result->bucketBy, AttributeReference("/~1foo~1bar"));
}

TEST(SegmentRuleTests, RejectsEmptyContextKind) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Rule, JsonError>>(boost::json::parse(
        R"({"rolloutContextKind" : "", "bucketBy" : "/~1foo~1bar", "clauses": []})"));
    ASSERT_FALSE(result);
}

TEST(SegmentRuleTests, DeserializesLiteralAttributeName) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Rule, JsonError>>(
        boost::json::parse(R"({"bucketBy" : "/~1foo~1bar", "clauses": []})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->bucketBy,
              AttributeReference::FromLiteralStr("/~1foo~1bar"));
}

TEST(ClauseTests, DeserializesMinimumValid) {
    auto result =
        boost::json::value_to<tl::expected<data_model::Clause, JsonError>>(
            boost::json::parse(R"({"op": "segmentMatch", "values": []})"));
    ASSERT_TRUE(result);

    ASSERT_EQ(result->op, data_model::Clause::Op::kSegmentMatch);
    ASSERT_TRUE(result->values.empty());
}

TEST(ClauseTests, TolerantOfUnrecognizedFields) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Clause, JsonError>>(boost::json::parse(
        R"({"somethingRandom": true, "attribute": "", "op": "in", "values": ["a"]})"));
    ASSERT_TRUE(result);
}

TEST(ClauseTests, TolerantOfEmptyAttribute) {
    auto result =
        boost::json::value_to<tl::expected<data_model::Clause, JsonError>>(
            boost::json::parse(
                R"({"attribute": "", "op": "segmentMatch", "values": ["a"]})"));
    ASSERT_TRUE(result);
    ASSERT_FALSE(result->attribute.Valid());
}

TEST(ClauseTests, TolerantOfUnrecognizedOperator) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Clause, JsonError>>(boost::json::parse(
        R"({"attribute": "", "op": "notAnActualOperator", "values": ["a"]})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->op, data_model::Clause::Op::kUnrecognized);
}

TEST(ClauseTests, DeserializesSimpleAttributeReference) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Clause, JsonError>>(boost::json::parse(
        R"({"attribute": "foo", "op": "in", "values": ["a"], "contextKind" : "user"})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->attribute, AttributeReference("foo"));
}

TEST(ClauseTests, DeserializesPointerAttributeReference) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Clause, JsonError>>(boost::json::parse(
        R"({"attribute": "/foo/bar", "op": "in", "values": ["a"], "contextKind" : "user"})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->attribute, AttributeReference("/foo/bar"));
}

TEST(ClauseTests, DeserializesEscapedReference) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Clause, JsonError>>(boost::json::parse(
        R"({"attribute": "/~1foo~1bar", "op": "in", "values": ["a"], "contextKind" : "user"})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->attribute, AttributeReference("/~1foo~1bar"));
}

TEST(ClauseTests, RejectsEmptyContextKind) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Clause, JsonError>>(boost::json::parse(
        R"({"attribute": "/~1foo~1bar", "op": "in", "values": ["a"], "contextKind" : ""})"));
    ASSERT_FALSE(result);
}

TEST(ClauseTests, DeserializesLiteralAttributeName) {
    auto result =
        boost::json::value_to<tl::expected<data_model::Clause, JsonError>>(
            boost::json::parse(
                R"({"attribute": "/foo/bar", "op": "in", "values": ["a"]})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->attribute,
              AttributeReference::FromLiteralStr("/foo/bar"));
}

TEST(RolloutTests, DeserializesMinimumValid) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::Rollout, JsonError>>(
        boost::json::parse(R"({})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->kind, data_model::Flag::Rollout::Kind::kRollout);
    ASSERT_EQ(result->contextKind, ContextKind("user"));
    ASSERT_EQ(result->bucketBy, "key");
}

TEST(RolloutTests, DeserializesAllFieldsWithAttributeReference) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::Rollout, JsonError>>(boost::json::parse(
        R"({"kind": "experiment", "contextKind": "org", "bucketBy": "/foo/bar", "seed" : 123, "variations" : []})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->kind, data_model::Flag::Rollout::Kind::kExperiment);
    ASSERT_EQ(result->contextKind, ContextKind("org"));
    ASSERT_EQ(result->bucketBy, "/foo/bar");
    ASSERT_EQ(result->seed, 123);
    ASSERT_TRUE(result->variations.empty());
}

TEST(RolloutTests, DeserializesAllFieldsWithLiteralAttributeName) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::Rollout, JsonError>>(boost::json::parse(
        R"({"kind": "experiment", "bucketBy": "/foo/bar", "seed" : 123, "variations" : []})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->kind, data_model::Flag::Rollout::Kind::kExperiment);
    ASSERT_EQ(result->contextKind, ContextKind("user"));
    ASSERT_EQ(result->bucketBy, "/~1foo~1bar");
    ASSERT_EQ(result->seed, 123);
    ASSERT_TRUE(result->variations.empty());
}

TEST(WeightedVariationTests, DeserializesMinimumValid) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::Rollout::WeightedVariation, JsonError>>(
        boost::json::parse(R"({})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->variation, 0);
    ASSERT_EQ(result->weight, 0);
    ASSERT_FALSE(result->untracked);
}

TEST(WeightedVariationTests, DeserializesAllFields) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::Rollout::WeightedVariation, JsonError>>(
        boost::json::parse(
            R"({"variation" : 2, "weight" : 123, "untracked" : true})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->variation, 2);
    ASSERT_EQ(result->weight, 123);
    ASSERT_TRUE(result->untracked);
}

TEST(PrerequisiteTests, DeserializeFailsWithoutKey) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::Prerequisite, JsonError>>(
        boost::json::parse(R"({})"));
    ASSERT_FALSE(result);
}

TEST(PrerequisiteTests, DeserializesMinimumValid) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::Prerequisite, JsonError>>(
        boost::json::parse(R"({"key" : "foo"})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->variation, 0);
    ASSERT_EQ(result->key, "foo");
}

TEST(PrerequisiteTests, DeserializesAllFields) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::Prerequisite, JsonError>>(
        boost::json::parse(R"({"key" : "foo", "variation" : 123})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->key, "foo");
    ASSERT_EQ(result->variation, 123);
}

TEST(PrerequisiteTests, DeserializeSucceedsWithNegativeVariation) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::Prerequisite, JsonError>>(
        boost::json::parse(R"({"key" : "foo", "variation" : -123})"));
    ASSERT_TRUE(result);
}

TEST(TargetTests, DeserializesMinimumValid) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::Target, JsonError>>(
        boost::json::parse(R"({})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->contextKind, ContextKind("user"));
    ASSERT_EQ(result->variation, 0);
    ASSERT_TRUE(result->values.empty());
}

TEST(TargetTests, DeserializesSucceedsWithNegativeVariation) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::Target, JsonError>>(
        boost::json::parse(R"({"variation" : -123})"));
    ASSERT_TRUE(result);
}

TEST(TargetTests, DeserializesAllFields) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::Target, JsonError>>(boost::json::parse(
        R"({"variation" : 123, "values" : ["a"], "contextKind" : "org"})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->contextKind, ContextKind("org"));
    ASSERT_EQ(result->variation, 123);
    ASSERT_EQ(result->values.size(), 1);
    ASSERT_EQ(result->values[0], "a");
}

TEST(FlagRuleTests, DeserializesMinimumValid) {
    auto result =
        boost::json::value_to<tl::expected<data_model::Flag::Rule, JsonError>>(
            boost::json::parse(R"({"variation" : 123})"));
    ASSERT_TRUE(result);
    ASSERT_FALSE(result->trackEvents);
    ASSERT_TRUE(result->clauses.empty());
    ASSERT_FALSE(result->id);
    ASSERT_EQ(std::get<std::optional<data_model::Flag::Variation>>(
                  result->variationOrRollout),
              data_model::Flag::Variation(123));
}

TEST(FlagRuleTests, DeserializesRollout) {
    auto result =
        boost::json::value_to<tl::expected<data_model::Flag::Rule, JsonError>>(
            boost::json::parse(R"({"rollout" : {}})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(
        std::get<data_model::Flag::Rollout>(result->variationOrRollout).kind,
        data_model::Flag::Rollout::Kind::kRollout);
}

TEST(FlagRuleTests, DeserializesAllFields) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::Rule, JsonError>>(boost::json::parse(
        R"({"id" : "foo", "variation" : 123, "trackEvents" : true, "clauses" : []})"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->trackEvents);
    ASSERT_TRUE(result->clauses.empty());
    ASSERT_EQ(result->id, "foo");
    ASSERT_EQ(std::get<std::optional<data_model::Flag::Variation>>(
                  result->variationOrRollout),
              data_model::Flag::Variation(123));
}

TEST(ClientSideAvailabilityTests, DeserializesMinimumValid) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::ClientSideAvailability, JsonError>>(
        boost::json::parse(R"({})"));
    ASSERT_TRUE(result);
    ASSERT_FALSE(result->usingMobileKey);
    ASSERT_FALSE(result->usingEnvironmentId);
}

TEST(ClientSideAvailabilityTests, DeserializesAllFields) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::ClientSideAvailability, JsonError>>(
        boost::json::parse(
            R"({"usingMobileKey" : true, "usingEnvironmentId" : true})"));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->usingMobileKey);
    ASSERT_TRUE(result->usingEnvironmentId);
}

TEST(WeightedVariationTests, SerializeAllFields) {
    data_model::Flag::Rollout::WeightedVariation variation(1, 2);
    variation.untracked = true;
    auto json = boost::json::value_from(variation);

    auto expected = boost::json::parse(
        R"({"variation": 1, "weight": 2, "untracked": true})");

    EXPECT_EQ(expected, json);
}

TEST(WeightedVariationTests, SerializeUntrackedOnlyTrue) {
    data_model::Flag::Rollout::WeightedVariation variation(1, 2);
    variation.untracked = false;
    auto json = boost::json::value_from(variation);

    auto expected = boost::json::parse(R"({"variation": 1, "weight": 2})");

    EXPECT_EQ(expected, json);
}

TEST(RolloutTests, SerializeAllFields) {
    using Rollout = data_model::Flag::Rollout;
    Rollout rollout;
    rollout.kind = Rollout::Kind::kExperiment;
    rollout.contextKind = "user";
    rollout.bucketBy = AttributeReference("ham");
    rollout.seed = 42;
    rollout.variations = {
        data_model::Flag::Rollout::WeightedVariation::Untracked(1, 2), {3, 4}};

    auto json = boost::json::value_from(rollout);

    auto expected = boost::json::parse(R"({
        "kind": "experiment",
        "contextKind": "user",
        "bucketBy": "ham",
        "seed": 42,
        "variations": [
            {"variation": 1, "weight": 2, "untracked": true},
            {"variation": 3, "weight": 4}
        ]
    })");

    EXPECT_EQ(expected, json);
}

TEST(VariationOrRolloutTests, SerializeVariation) {
    data_model::Flag::VariationOrRollout variation = 5;


    //Explanation in value_mapping.hpp.
#if BOOST_VERSION >= 108300
    auto json = boost::json::value_from(variation, VariationOrRolloutContext());
#else
    auto json = boost::json::value_from(var_or_roll);
#endif
    auto expected = boost::json::parse(R"({"variation":5})");
    EXPECT_EQ(expected, json);
}

TEST(VariationOrRolloutTests, SerializeRollout) {
    using Rollout = data_model::Flag::Rollout;
    Rollout rollout;
    rollout.kind = Rollout::Kind::kExperiment;
    rollout.contextKind = "user";
    rollout.bucketBy = AttributeReference("ham");
    rollout.seed = 42;
    rollout.variations = {
        data_model::Flag::Rollout::WeightedVariation::Untracked(1, 2), {3, 4}};
    data_model::Flag::VariationOrRollout var_or_roll;
    var_or_roll.emplace<Rollout>(rollout);
    //Explanation in value_mapping.hpp.
#if BOOST_VERSION >= 108300
    auto json = boost::json::value_from(var_or_roll, VariationOrRolloutContext());
#else
    auto json = boost::json::value_from(var_or_roll);
#endif


    auto expected = boost::json::parse(R"({
    "rollout":{
        "kind": "experiment",
        "contextKind": "user",
        "bucketBy": "ham",
        "seed": 42,
        "variations": [
            {"variation": 1, "weight": 2, "untracked": true},
            {"variation": 3, "weight": 4}
        ]
    }})");
    EXPECT_EQ(expected, json);
}

TEST(PrerequisiteTests, SerializeAll) {
    data_model::Flag::Prerequisite prerequisite{"potato", 6};
    auto json = boost::json::value_from(prerequisite);

    auto expected = boost::json::parse(R"({"key":"potato","variation":6})");
    EXPECT_EQ(expected, json);
}

TEST(TargetTests, SerializeAll) {
    data_model::Flag::Target target{{"a", "b"}, 42, ContextKind("taco_stand")};
    auto json = boost::json::value_from(target);

    auto expected = boost::json::parse(
        R"({"values":["a", "b"], "variation": 42, "contextKind":"taco_stand"})");
    EXPECT_EQ(expected, json);
}

TEST(ClientSideAvailabilityTests, SerializeAll) {
    data_model::Flag::ClientSideAvailability availability{true, true};

    auto json = boost::json::value_from(availability);

    auto expected = boost::json::parse(
        R"({"usingMobileKey": true, "usingEnvironmentId": true})");
    EXPECT_EQ(expected, json);
}

class ClauseOperatorsFixture
    : public ::testing::TestWithParam<data_model::Clause::Op> {};

INSTANTIATE_TEST_SUITE_P(
    ClauseTests,
    ClauseOperatorsFixture,
    testing::Values(data_model::Clause::Op::kSegmentMatch,
                    data_model::Clause::Op::kAfter,
                    data_model::Clause::Op::kBefore,
                    data_model::Clause::Op::kContains,
                    data_model::Clause::Op::kEndsWith,
                    data_model::Clause::Op::kGreaterThan,
                    data_model::Clause::Op::kGreaterThanOrEqual,
                    data_model::Clause::Op::kIn,
                    data_model::Clause::Op::kLessThan,
                    data_model::Clause::Op::kLessThanOrEqual,
                    data_model::Clause::Op::kMatches,
                    data_model::Clause::Op::kSemVerEqual,
                    data_model::Clause::Op::kSemVerGreaterThan,
                    data_model::Clause::Op::kSemVerLessThan,
                    data_model::Clause::Op::kStartsWith));

TEST_P(ClauseOperatorsFixture, AllOperatorsSerializeDeserialize) {
    auto op = GetParam();

    auto serialized = boost::json::serialize(boost::json::value_from(op));
    auto parsed = boost::json::parse(serialized);
    auto deserialized = boost::json::value_to<
        tl::expected<std::optional<data_model::Clause::Op>, JsonError>>(parsed);

    EXPECT_EQ(op, **deserialized);
}

TEST(ClauseTests, SerializeAll) {
    data_model::Clause clause{data_model::Clause::Op::kIn,
                              {"a", "b"},
                              true,
                              ContextKind("bob"),
                              "/potato"};

    auto json = boost::json::value_from(clause);
    auto expected = boost::json::parse(
        R"({
            "op": "in",
            "negate": true,
            "values": ["a", "b"],
            "contextKind": "bob",
            "attribute": "/potato"
        })");
    EXPECT_EQ(expected, json);
}

TEST(FlagRuleTests, SerializeAllRollout) {
    using Rollout = data_model::Flag::Rollout;
    Rollout rollout;
    rollout.kind = Rollout::Kind::kExperiment;
    rollout.contextKind = "user";
    rollout.bucketBy = AttributeReference("ham");
    rollout.seed = 42;
    rollout.variations = {
        data_model::Flag::Rollout::WeightedVariation::Untracked(1, 2), {3, 4}};
    data_model::Flag::Rule rule{{{data_model::Clause::Op::kIn,
                                  {"a", "b"},
                                  true,
                                  ContextKind("bob"),
                                  "/potato"}},
                                rollout,
                                true,
                                "therule"};

    auto json = boost::json::value_from(rule);
    auto expected = boost::json::parse(
        R"({
            "clauses":[{
                "op": "in",
                "negate": true,
                "values": ["a", "b"],
                "contextKind": "bob",
                "attribute": "/potato"
            }],
            "rollout": {
                "kind": "experiment",
                "contextKind": "user",
                "bucketBy": "ham",
                "seed": 42,
                "variations": [
                    {"variation": 1, "weight": 2, "untracked": true},
                    {"variation": 3, "weight": 4}
                ]
            },
            "trackEvents": true,
            "id": "therule"
        })");
    EXPECT_EQ(expected, json);
}

TEST(FlagTests, SerializeAll) {
    data_model::Flag flag{
        "the-key",
        21,                                // version
        true,                              // on
        42,                                // fallthrough
        {"a", "b"},                        // variations
        {{"prereqA", 2}, {"prereqB", 3}},  // prerequisites
        {{{
              "1",
              "2",
              "3",
          },
          12,
          ContextKind("user")}},  // targets
        {{{
              "4",
              "5",
              "6",
          },
          24,
          ContextKind("bob")}},  // contextTargets
        {data_model::Flag::Rule{{data_model::Clause{data_model::Clause::Op::kIn,
                                                    {"bob"},
                                                    false,
                                                    ContextKind{"user"},
                                                    "name"}}}},  // rules
        84,                                                      // offVariation
        true,                                                    // clientSide
        data_model::Flag::ClientSideAvailability{true, true},
        "4242",  // salt
        true,    // trackEvents
        true,    // trackEventsFalltrhough
        900      // debugEventsUntilDate
    };

    auto json = boost::json::value_from(flag);
    auto expected = boost::json::parse(
        R"({
            "trackEvents":true,
            "clientSide":true,
            "on":true,
            "trackEventsFallthrough":true,
            "debugEventsUntilDate":900,
            "salt":"4242",
            "offVariation":84,
            "key":"the-key",
            "version":21,
            "variations":["a","b"],
            "rules":[{"clauses":[{"op":"in","values":["bob"],"contextKind":"user","attribute":"name"}]}],
            "prerequisites":[{"key":"prereqA","variation":2},
                {"key":"prereqB","variation":3}],
            "fallthrough":{"variation":42},
            "clientSideAvailability":
                {"usingEnvironmentId":true,"usingMobileKey":true},
            "contextTargets":
                [{"values":["4","5","6"],"variation":24,"contextKind":"bob"}],
            "targets":[{"values":["1","2","3"],"variation":12}]
    })");
    EXPECT_EQ(expected, json);
}

TEST(SegmentTargetTests, SerializeAll) {
    data_model::Segment::Target target{ContextKind("bob"), {"bill", "sam"}};

    auto json = boost::json::value_from(target);
    auto expected = boost::json::parse(
        R"({
            "contextKind": "bob",
            "values": ["bill", "sam"]
        })");
    EXPECT_EQ(expected, json);
}

TEST(SegmentRuleTests, SerializeAll) {
    data_model::Segment::Rule rule{{{data_model::Clause::Op::kIn,
                                     {"a", "b"},
                                     true,
                                     ContextKind("bob"),
                                     "/potato"}},
                                   "ididid",
                                   300,
                                   ContextKind("bob"),
                                   "/happy"};

    auto json = boost::json::value_from(rule);
    auto expected = boost::json::parse(
        R"({
            "clauses": [{
                "op": "in",
                "negate": true,
                "values": ["a", "b"],
                "contextKind": "bob",
                "attribute": "/potato"
            }],
            "id": "ididid",
            "weight": 300,
            "rolloutContextKind": "bob",
            "bucketBy": "/happy"
        })");
    EXPECT_EQ(expected, json);
}

TEST(SegmentTests, SerializeBasicAll) {
    data_model::Segment segment{
        "my-segment",
        87,
        {"bob", "sam"},
        {"sally", "johan"},
        {{ContextKind("vegetable"), {"potato", "yam"}}},
        {{ContextKind("material"), {"cardboard", "plastic"}}},
        {{{{data_model::Clause::Op::kIn,
            {"a", "b"},
            true,
            ContextKind("bob"),
            "/potato"}},
          "ididid",
          300,
          ContextKind("bob"),
          "/happy"}},
        "salty",
        false,
        std::nullopt,
        std::nullopt,
    };

    auto json = boost::json::value_from(segment);
    auto expected = boost::json::parse(
        R"({
            "key": "my-segment",
            "version": 87,
            "included": ["bob", "sam"],
            "excluded": ["sally", "johan"],
            "includedContexts":
                [{"contextKind": "vegetable", "values":["potato", "yam"]}],
            "excludedContexts":
                [{"contextKind": "material", "values":["cardboard", "plastic"]}],
            "salt": "salty",
            "rules":[{
                "clauses": [{
                    "op": "in",
                    "negate": true,
                    "values": ["a", "b"],
                    "contextKind": "bob",
                    "attribute": "/potato"
                }],
                "id": "ididid",
                "weight": 300,
                "rolloutContextKind": "bob",
                "bucketBy": "/happy"
            }]
    })");
    EXPECT_EQ(expected, json);
}

TEST(SegmentTests, SerializeUnbounded) {
    data_model::Segment segment{
        "my-segment",           87, {}, {}, {}, {}, {}, "salty", true,
        ContextKind("company"), 12};

    auto json = boost::json::value_from(segment);
    auto expected = boost::json::parse(
        R"({
            "key": "my-segment",
            "version": 87,
            "salt": "salty",
            "unbounded": true,
            "unboundedContextKind": "company",
            "generation": 12
    })");
    EXPECT_EQ(expected, json);
}
