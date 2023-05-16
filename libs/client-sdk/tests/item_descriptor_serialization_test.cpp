#include <gtest/gtest.h>

#include <unordered_map>
#include <memory>

#include "serialization/json_item_descriptor.hpp"

#include <launchdarkly/data/evaluation_result.hpp>

using launchdarkly::EvaluationDetailInternal;
using launchdarkly::EvaluationReason;
using launchdarkly::EvaluationResult;
using launchdarkly::Value;
using launchdarkly::client_side::ItemDescriptor;

TEST(ItemDescriptorSerialization, CanSerializeFullItemDescritor) {
    EvaluationReason reason(EvaluationReason::Kind::kOff,
                            EvaluationReason::ErrorKind::kMalformedFlag, 12,
                            "RULE_ID", "PREREQ_KEY", true, "STORE_ERROR");
    EvaluationDetailInternal detail(
        Value(std::map<std::string, Value>{{"item", "a"}}), 84, reason);
    EvaluationResult result(12, 24, true, true,
                            std::chrono::system_clock::time_point{
                                std::chrono::milliseconds{1680555761}},
                            detail);

    ItemDescriptor descriptor(result);
    auto res = boost::json::serialize(boost::json::value_from(descriptor));

    EXPECT_EQ(
        "{\"version\":12,"
        "\"flag\":{\"version\":12,\"flagVersion\":24,\"trackEvents\":true,"
        "\"trackReason\":true,\"debugEventsUntilDate\":1680555761,\"value\":{"
        "\"item\":\"a\"},\"variationIndex\":84,\"reason\":{\"kind\":\"OFF\","
        "\"errorKind\":\"MALFORMED_FLAG\",\"bigSegmentStatus\":\"STORE_ERROR\","
        "\"ruleId\":\"RULE_ID\",\"ruleIndex\":12,\"inExperiment\":true,"
        "\"prerequisiteKey\":\"PREREQ_KEY\"}}}",
        res);
}

TEST(ItemDescriptorSerialization, CanSerializeDeletedItemDescritor) {
    ItemDescriptor descriptor(42);
    auto res = boost::json::serialize(boost::json::value_from(descriptor));

    EXPECT_EQ(R"({"version":42})", res);
}

TEST(ItemDescriptorSerialization, CanSerializeMap) {
    auto items = std::unordered_map<std::string, std::shared_ptr<ItemDescriptor>>();
    items.emplace("A", std::make_shared<ItemDescriptor>(1));
    items.emplace("B", std::make_shared<ItemDescriptor>(2));

    auto res = boost::json::serialize(boost::json::value_from(items));
    //TODO: Implement
}