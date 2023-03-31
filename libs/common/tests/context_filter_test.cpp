#include <gtest/gtest.h>

#include "attribute_reference.hpp"
#include "context_builder.hpp"
#include "context_filter.hpp"

using launchdarkly::AttributeReference;
using launchdarkly::ContextBuilder;
using launchdarkly::ContextFilter;
using launchdarkly::Value;
using Object = launchdarkly::Value::Object;

TEST(ContextFilterTests, BasicContext) {
    ContextFilter filter(false, AttributeReference::SetType());

    auto filtered = filter.filter(ContextBuilder()
                                      .kind("user", "user-key")
                                      .set("email", "email.email@email")
                                      .set("array", {false, true, "bacon"})
                                      .set("object", Object{{"test", true}})
                                      .build());
    std::cout << filtered << std::endl;
    EXPECT_EQ(true, false);
}