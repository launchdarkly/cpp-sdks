#include <gtest/gtest.h>

#include <boost/json.hpp>
#include <launchdarkly/attribute_reference.hpp>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/context_filter.hpp>

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
                                      .Kind("user", "user-key")
                                      .Name("Bob")
                                      .Set("isCat", false)
                                      .Build());

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
                                      .Kind("user", "user-key")
                                      .Name("Bob")
                                      .Set("isCat", false)
                                      .Build());

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
            .Kind("user", "user-key")
            .Set("email", "email.email@email")
            .Set("array", {false, true, "bacon", {"first", "second"}})
            .Set("object",
                 Object{{"test", true}, {"second", false}, {"arr", {1, 2}}})
            .Set("arrayWithObject", {true, Object{{"a", "b"}}, "c"})
            .SetPrivate("isCat", false)
            .AddPrivateAttribute("/object/test")
            .Build());

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
            .Kind("user", "user-key")
            .Set("email", "email.email@email")
            .Set("array", {false, true, "bacon"})
            .Set("object", Object{{"test", true}, {"second", false}})
            .SetPrivate("isCat", false)
            .AddPrivateAttribute("/object/test"
            .Kind("org", "org-key")
            .Set("isCat", true)           // Not filtered in this context.
            .Set("email", "cat@cat.cat")  // Filtered globally.
            .Build());

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
                                      .Kind("user", "user-key")
                                      .Name("Bob")
                                      .Set("isCat", false)
                                      .Build());

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
            .Kind("user", "user-key")
            .Name("Bob")
            .Set("email", "email.email@email")
            .Set("array", {false, true, "bacon"})
            .Set("object", Object{{"test", true}, {"second", false}})
            .SetPrivate("isCat", false)
            .AddPrivateAttribute("/object/test")
            .Kind("org", "org-key")
            .Set("isCat", true)
            .Set("email", "cat@cat.cat")
            .Build());

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
