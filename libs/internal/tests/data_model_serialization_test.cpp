#include <gtest/gtest.h>

#include <launchdarkly/serialization/json_sdk_data_set.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

using namespace launchdarkly;

TEST(SDKDataSetTests, DeserializesEmptyDataSet) {
    auto result =
        boost::json::value_to<tl::expected<data_model::SDKDataSet, JsonError>>(
            boost::json::parse("{}"));
    ASSERT_TRUE(result);
    ASSERT_FALSE(result->segments);
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
    ASSERT_TRUE(result->segments->empty());
}

TEST(SDKDataSetTests, DeserializesComplexSegment) {
    auto result =
        boost::json::value_to<tl::expected<data_model::SDKDataSet, JsonError>>(
            boost::json::parse(R"({"segments":{
"foo" : {"key":"foo", "version": 42, "included": {"values": ["a", "b"]}, "excluded": {"values": ["c", "d"]}, "salt": "salt", "rules": [{"clauses": [{"attribute": "foo", "op": "in", "values": ["bar"]}]}]}}})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->segments->size(), 1);
    auto const& segment_descriptor = result->segments->at("foo");

    ASSERT_TRUE(segment_descriptor.item);
    ASSERT_EQ(segment_descriptor.version, 42);

    auto const& segment = *segment_descriptor.item;
    ASSERT_EQ(segment.key, "foo");
    ASSERT_EQ(segment.salt, "salt");
}

TEST(SegmentTests, DeserializesMinimumValid) {
    auto result =
        boost::json::value_to<tl::expected<data_model::Segment, JsonError>>(
            boost::json::parse(R"({"key":"foo", "version": 42})"));
    ASSERT_TRUE(result);

    ASSERT_EQ(result->version, 42);
    ASSERT_EQ(result->key, "foo");
}

TEST(SegmentTests, TolerantOfUnrecognizedFields) {
    auto result =
        boost::json::value_to<tl::expected<data_model::Segment, JsonError>>(
            boost::json::parse(
                R"({"key":"foo", "version": 42, "somethingRandom" : true})"));
    ASSERT_TRUE(result);
}

TEST(RuleTests, DeserializesMinimumValid) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Rule, JsonError>>(boost::json::parse(
        R"({"clauses": [{"attribute": "", "op": "in", "values": []}]})"));
    ASSERT_TRUE(result);

    auto const& clauses = result->clauses;
    ASSERT_EQ(clauses.size(), 1);

    auto const& clause = clauses.at(0);
    ASSERT_EQ(clause.op, data_model::Segment::Clause::Op::kIn);
}

TEST(RuleTests, TolerantOfUnrecognizedFields) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Rule, JsonError>>(boost::json::parse(
        R"({"somethingRandom": true, "clauses": [{"attribute": "", "op": "in", "values": []}]})"));

    ASSERT_TRUE(result);
}

TEST(RuleTests, DeserializesSimpleAttributeReference) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Rule, JsonError>>(boost::json::parse(
        R"({"rolloutContextKind" : "foo", "bucketBy" : "bar", "clauses": []})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->rolloutContextKind, "foo");
    ASSERT_EQ(result->bucketBy, AttributeReference("bar"));
}

TEST(RuleTests, DeserializesPointerAttributeReference) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Rule, JsonError>>(boost::json::parse(
        R"({"rolloutContextKind" : "foo", "bucketBy" : "/foo/bar", "clauses": []})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->rolloutContextKind, "foo");
    ASSERT_EQ(result->bucketBy, AttributeReference("/foo/bar"));
}

TEST(RuleTests, DeserializesEscapedReference) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Rule, JsonError>>(boost::json::parse(
        R"({"rolloutContextKind" : "foo", "bucketBy" : "/~1foo~1bar", "clauses": []})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->rolloutContextKind, "foo");
    ASSERT_EQ(result->bucketBy, AttributeReference("/~1foo~1bar"));
}

TEST(RuleTests, DeserializesLiteralReference) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Rule, JsonError>>(
        boost::json::parse(R"({"bucketBy" : "/~1foo~1bar", "clauses": []})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->bucketBy,
              AttributeReference::FromLiteralStr("/~1foo~1bar"));
}

TEST(ClauseTests, DeserializesMinimumValid) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Clause, JsonError>>(
        boost::json::parse(R"({"attribute": "", "op": "in", "values": []})"));
    ASSERT_TRUE(result);

    ASSERT_EQ(result->op, data_model::Segment::Clause::Op::kIn);
    ASSERT_TRUE(result->values.empty());
    ASSERT_FALSE(result->attribute.Valid());
}

TEST(ClauseTests, TolerantOfUnrecognizedFields) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Clause, JsonError>>(boost::json::parse(
        R"({"somethingRandom": true, "attribute": "", "op": "in", "values": []})"));
    ASSERT_TRUE(result);
}

TEST(ClauseTests, TolerantOfEmptyAttribute) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Clause, JsonError>>(
        boost::json::parse(R"({"attribute": "", "op": "in", "values": []})"));
    ASSERT_TRUE(result);
    ASSERT_FALSE(result->attribute.Valid());
}

TEST(ClauseTests, TolerantOfUnrecognizedOperator) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Clause, JsonError>>(
        boost::json::parse(
            R"({"attribute": "", "op": "notAnActualOperator", "values": []})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->op, data_model::Segment::Clause::Op::kUnrecognized);
}

TEST(ClauseTests, DeserializesSimpleAttributeReference) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Clause, JsonError>>(boost::json::parse(
        R"({"attribute": "foo", "op": "in", "values": [], "contextKind" : "user"})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->attribute, AttributeReference("foo"));
}

TEST(ClauseTests, DeserializesPointerAttributeReference) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Clause, JsonError>>(boost::json::parse(
        R"({"attribute": "/foo/bar", "op": "in", "values": [], "contextKind" : "user"})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->attribute, AttributeReference("/foo/bar"));
}

TEST(ClauseTests, DeserializesEscapedReference) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Clause, JsonError>>(boost::json::parse(
        R"({"attribute": "/~1foo~1bar", "op": "in", "values": [], "contextKind" : "user"})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->attribute, AttributeReference("/~1foo~1bar"));
}

TEST(ClauseTests, DeserializesLiteralAttributeReference) {
    auto result = boost::json::value_to<
        tl::expected<data_model::Segment::Clause, JsonError>>(
        boost::json::parse(
            R"({"attribute": "/foo/bar", "op": "in", "values": []})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->attribute,
              AttributeReference::FromLiteralStr("/foo/bar"));
}
