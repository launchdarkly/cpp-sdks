#include <gtest/gtest.h>

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_primitives.hpp>
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

TEST(PrerequisiteTests, DeserializeFailsWithNegativeVariation) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::Prerequisite, JsonError>>(
        boost::json::parse(R"({"key" : "foo", "variation" : -123})"));
    ASSERT_FALSE(result);
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

TEST(TargetTests, DeserializesFailsWithNegativeVariation) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Flag::Target, JsonError>>(
        boost::json::parse(R"({"variation" : -123})"));
    ASSERT_FALSE(result);
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
    ASSERT_EQ(std::get<data_model::Flag::Variation>(result->variationOrRollout),
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
    ASSERT_EQ(std::get<data_model::Flag::Variation>(result->variationOrRollout),
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
