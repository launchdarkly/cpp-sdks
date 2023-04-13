#include <gtest/gtest.h>

#include <boost/json.hpp>

#include "context_builder.hpp"
#include "serialization/json_context.hpp"

using launchdarkly::Context;
using launchdarkly::ContextBuilder;

TEST(ContextTests, CanonicalKeyForUser) {
    auto context = ContextBuilder().kind("user", "user-key").build();
    EXPECT_EQ("user-key", context.canonical_key());
}

TEST(ContextTests, CanonicalKeyForNonUser) {
    auto context = ContextBuilder().kind("org", "org-key").build();
    EXPECT_EQ("org:org-key", context.canonical_key());
}

TEST(ContextTests, CanonicalKeyForMultiContext) {
    auto context = ContextBuilder()
                       .kind("user", "user-key")
                       .kind("org", "org-key")
                       .build();

    EXPECT_EQ("org:org-key:user:user-key", context.canonical_key());
}

TEST(ContextTests, EscapesCanonicalKey) {
    auto context = ContextBuilder()
                       .kind("user", "user:key")
                       .kind("org", "org%key")
                       .build();

    EXPECT_EQ("org:org%3Akey:user:user%25key", context.canonical_key());
}

TEST(ContextTests, CanGetKeysAndKinds) {
    auto context = ContextBuilder()
                       .kind("user", "user-key")
                       .kind("org", "org-key")
                       .build();
    EXPECT_EQ(2, context.kinds_to_keys().size());
    EXPECT_EQ("user-key", context.kinds_to_keys().find("user")->second);
    EXPECT_EQ("org-key", context.kinds_to_keys().find("org")->second);
}

std::string ProduceString(Context const& ctx) {
    std::stringstream stream;
    stream << ctx;
    stream.flush();
    return stream.str();
}

TEST(ContextTests, OstreamOperatorValidContext) {
    auto context = ContextBuilder().kind("user", "user-key").build();
    EXPECT_EQ(
        "{contexts: [kind: user attributes: {key: string(user-key),  name: "
        "string() anonymous: bool(false) private: []  custom: object({})}]",
        ProduceString(context));

    auto context_2 = ContextBuilder()
                         .kind("user", "user-key")
                         .kind("org", "org-key")
                         .name("Sam")
                         .set_private("test", true)
                         .set("string", "potato")
                         .build();

    EXPECT_EQ(
        "{contexts: [kind: org attributes: {key: string(org-key),  name: "
        "string(Sam) anonymous: bool(false) private: [valid(test)]  custom: "
        "object({{string, string(potato)}, {test, bool(true)}})}, kind: user "
        "attributes: {key: string(user-key),  name: string() anonymous: "
        "bool(false) private: []  custom: object({})}]",
        ProduceString(context_2));
}

TEST(ContextTests, OstreamOperatorInvalidContext) {
    auto context = ContextBuilder().kind("#$#*(", "").build();
    EXPECT_FALSE(context.valid());

    EXPECT_EQ(
        "{invalid: errors: [#$#*(: \"Kind contained invalid characters. A kind "
        "may contain ASCII letters or numbers, as well as '.', '-', and "
        "'_'.\", "
        "#$#*(: \"The key for a context may not be empty.\"]",
        ProduceString(context));
}

// The attributes_test covers most serialization conditions.
// So these tests are more for overall shape of single and multi-contexts.
TEST(ContextTests, JsonSerializeSingleContext) {
    auto context_value =
        boost::json::value_from(ContextBuilder()
                                    .kind("user", "user-key")
                                    .set("isCat", true)
                                    .set_private("email", "cat@email.email")
                                    .build());

    auto parsed_value = boost::json::parse(
        "{"
        "\"kind\":\"user\","
        "\"key\":\"user-key\","
        "\"isCat\":true,"
        "\"email\":\"cat@email.email\","
        "\"_meta\":{\"privateAttributes\": [\"email\"]}"
        "}");

    EXPECT_EQ(parsed_value, context_value);
}

TEST(ContextTests, JsonSerializeMultiContext) {
    auto context_value =
        boost::json::value_from(ContextBuilder()
                                    .kind("user", "user-key")
                                    .set("isCat", true)
                                    .set_private("email", "cat@email.email")
                                    .kind("org", "org-key")
                                    .build());

    auto parsed_value = boost::json::parse(
        "{"
        "\"kind\":\"multi\","
        "\"user\":{"
        "\"key\":\"user-key\","
        "\"isCat\":true,"
        "\"email\":\"cat@email.email\","
        "\"_meta\":{\"privateAttributes\": [\"email\"]}"
        "},"
        "\"org\":{"
        "\"key\":\"org-key\""
        "}"
        "}");

    EXPECT_EQ(parsed_value, context_value);
}
