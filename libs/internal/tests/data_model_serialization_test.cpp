#include <gtest/gtest.h>

#include <launchdarkly/serialization/json_sdk_data_set.hpp>

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

TEST(SDKDataSetTests, DeserializesMinimumValidSegment) {
    auto result =
        boost::json::value_to<tl::expected<data_model::SDKDataSet, JsonError>>(
            boost::json::parse(
                R"({"segments":{"foo":{"key":"foo", "version": 42}}})"));
    ASSERT_TRUE(result);
    ASSERT_EQ(result->segments->size(), 1);

    auto it = result->segments->find("foo");
    ASSERT_TRUE(it != result->segments->end());

    ASSERT_EQ(it->second.version, 42);
    ASSERT_TRUE(it->second.item);

    ASSERT_EQ(it->second.item->key, "foo");
    ASSERT_EQ(it->second.item->version, 42);
}
