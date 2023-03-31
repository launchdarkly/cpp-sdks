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
    auto global_private_attributes = AttributeReference::SetType{"email"};
    ContextFilter filter(false, global_private_attributes);

    auto filtered = filter.filter(ContextBuilder()
                                      .kind("user", "user-key")
                                      .set("email", "email.email@email")
                                      .set("array", {false, true, "bacon"})
                                      .set("object", Object{{"test", true}, {"second", false}})
                                      .set_private("isCat", false)
                                      .add_private_attribute("/object/test")
                                      .build());
    std::cout << filtered << std::endl;
    EXPECT_EQ(true, false);
}