#include <gtest/gtest.h>

#include <boost/json.hpp>
#include "launchdarkly/attribute_reference.hpp"
#include "launchdarkly/context_builder.hpp"
#include "launchdarkly/context_filter.hpp"

using launchdarkly::AttributeReference;
using launchdarkly::ContextBuilder;
using launchdarkly::ContextFilter;
using launchdarkly::Value;
using Object = launchdarkly::Value::Object;

using boost::json::parse;

TEST(ContextFilterTests, CanRedactName) {
    auto global_private_attributes = AttributeReference::SetType{"name"};
    ContextFilter filter(false, global_private_attributes);

    auto filtered = filter.filter(ContextBuilder()
                                      .kind("user", "user-key")
                                      .name("Bob")
                                      .set("isCat", false)
                                      .build());

    auto expected = parse(
        "{\"key\":\"user-key\","
        "\"kind\":\"user\","
        "\"isCat\":false,"
        "\"_meta\":{"
        "\"redactedAttributes\":[\"name\"]}}");

    EXPECT_EQ(expected, filtered);
}

TEST(ContextFilterTests, CannotRedactKeyKindMeta) {
    auto global_private_attributes =
        AttributeReference::SetType{"key", "kind", "_meta", "name"};
    ContextFilter filter(false, global_private_attributes);

    auto filtered = filter.filter(ContextBuilder()
                                      .kind("user", "user-key")
                                      .name("Bob")
                                      .set("isCat", false)
                                      .build());

    auto expected = parse(
        "{\"key\":\"user-key\","
        "\"kind\":\"user\","
        "\"isCat\":false,"
        "\"_meta\":{"
        "\"redactedAttributes\":[\"name\"]}}");

    EXPECT_EQ(expected, filtered);
}

TEST(ContextFilterTests, BasicContext) {
    auto global_private_attributes = AttributeReference::SetType{"email"};
    ContextFilter filter(false, global_private_attributes);

    auto filtered = filter.filter(
        ContextBuilder()
            .kind("user", "user-key")
            .set("email", "email.email@email")
            .set("array", {false, true, "bacon", {"first", "second"}})
            .set("object",
                 Object{{"test", true}, {"second", false}, {"arr", {1, 2}}})
            .set("arrayWithObject", {true, Object{{"a", "b"}}, "c"})
            .set_private("isCat", false)
            .add_private_attribute("/object/test")
            .build());

    auto expected = parse(
        "{"
        "\"key\":\"user-key\""
        ",\"kind\":\"user\","
        "\"object\":"
        "{"
        "\"second\":false,\"arr\":[1E0,2E0]"
        "},"
        "\"arrayWithObject\":[true,{\"a\":\"b\"},\"c\"],"
        "\"array\":[false,true,\"bacon\",[\"first\",\"second\"]],"
        "\"_meta\":{"
        "\"redactedAttributes\":[\"/object/test\",\"isCat\",\"email\"]}"
        "}");

    EXPECT_EQ(expected, filtered);
}

TEST(ContextFilterTests, MultiContext) {
    auto global_private_attributes = AttributeReference::SetType{"email"};
    ContextFilter filter(false, global_private_attributes);

    auto filtered = filter.filter(
        ContextBuilder()
            .kind("user", "user-key")
            .set("email", "email.email@email")
            .set("array", {false, true, "bacon"})
            .set("object", Object{{"test", true}, {"second", false}})
            .set_private("isCat", false)
            .add_private_attribute("/object/test")
            .kind("org", "org-key")
            .set("isCat", true)           // Not filtered in this context.
            .set("email", "cat@cat.cat")  // Filtered globally.
            .build());

    auto expected = parse(
        "{"
        "\"kind\":\"multi\","
        "\"org\":"
        "{"
        "\"key\":\"org-key\","
        "\"isCat\":true,"
        "\"_meta\":{\"redactedAttributes\":[\"email\"]}}"
        ",\"user\":"
        "{"
        "\"key\":\"user-key\","
        "\"object\":{\"second\":false},"
        "\"array\":[false,true,\"bacon\"],"
        "\"_meta\":{"
        "\"redactedAttributes\":[\"/object/test\",\"isCat\",\"email\"]}}}");

    EXPECT_EQ(expected, filtered);
}

TEST(ContextFilterTests, AllAttributesPrivateSingleContext) {
    auto global_private_attributes = AttributeReference::SetType{};
    ContextFilter filter(true, global_private_attributes);

    auto filtered = filter.filter(ContextBuilder()
                                      .kind("user", "user-key")
                                      .name("Bob")
                                      .set("isCat", false)
                                      .build());

    auto expected = parse(
        "{\"key\":\"user-key\","
        "\"kind\":\"user\","
        "\"_meta\":{"
        "\"redactedAttributes\":[\"/name\", \"/isCat\"]}}");

    EXPECT_EQ(expected, filtered);
}

TEST(ContextFilterTests, AllAttributesPrivateMultiContext) {
    auto global_private_attributes = AttributeReference::SetType{};
    ContextFilter filter(true, global_private_attributes);

    auto filtered = filter.filter(
        ContextBuilder()
            .kind("user", "user-key")
            .name("Bob")
            .set("email", "email.email@email")
            .set("array", {false, true, "bacon"})
            .set("object", Object{{"test", true}, {"second", false}})
            .set_private("isCat", false)
            .add_private_attribute("/object/test")
            .kind("org", "org-key")
            .set("isCat", true)
            .set("email", "cat@cat.cat")
            .build());

    auto expected = parse(
        "{\"kind\":\"multi\""
        ",\"org\":"
        "{\"key\":\"org-key\",\"_meta\":{"
        "\"redactedAttributes\":[\"/isCat\",\"/"
        "email\"]}},"
        "\"user\":"
        "{\"key\":\"user-key\",\"_meta\":{"
        "\"redactedAttributes\":[\"/name\",\"/object\",\"/isCat\",\"/"
        "email\",\"/array\"]}}}");

    EXPECT_EQ(expected, filtered);
}
