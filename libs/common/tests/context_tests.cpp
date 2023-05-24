#include <gtest/gtest.h>

#include <boost/json.hpp>

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/serialization/json_context.hpp>

using launchdarkly::Context;
using launchdarkly::ContextBuilder;

TEST(ContextTests, CanonicalKeyForUser) {
    auto context = ContextBuilder().Kind("user", "user-key").Build();
    EXPECT_EQ("user-key", context.CanonicalKey());
}

TEST(ContextTests, CanonicalKeyForNonUser) {
    auto context = ContextBuilder().Kind("org", "org-key").Build();
    EXPECT_EQ("org:org-key", context.CanonicalKey());
}

TEST(ContextTests, CanonicalKeyForMultiContext) {
    auto context = ContextBuilder()
                       .Kind("user", "user-key")
                       .Kind("org", "org-key")
                       .Build();

    EXPECT_EQ("org:org-key:user:user-key", context.CanonicalKey());
}

TEST(ContextTests, EscapesCanonicalKey) {
    auto context = ContextBuilder()
                       .Kind("user", "user:key")
                       .Kind("org", "org%key")
                       .Build();

    EXPECT_EQ("org:org%3Akey:user:user%25key", context.CanonicalKey());
}

TEST(ContextTests, CanGetKeysAndKinds) {
    auto context = ContextBuilder()
                       .Kind("user", "user-key")
                       .Kind("org", "org-key")
                       .Build();
    EXPECT_EQ(2, context.KindsToKeys().size());
    EXPECT_EQ("user-key", context.KindsToKeys().find("user")->second);
    EXPECT_EQ("org-key", context.KindsToKeys().find("org")->second);
}

std::string ProduceString(Context const& ctx) {
    std::stringstream stream;
    stream << ctx;
    stream.flush();
    return stream.str();
}

TEST(ContextTests, OstreamOperatorValidContext) {
    auto context = ContextBuilder().Kind("user", "user-key").Build();
    EXPECT_EQ(
        "{contexts: [kind: user attributes: {key: string(user-key),  name: "
        "string() anonymous: bool(false) private: []  custom: object({})}]",
        ProduceString(context));

    auto context_2 = ContextBuilder()
                         .Kind("user", "user-key")
                         .Kind("org", "org-key")
                         .Name("Sam")
                         .SetPrivate("test", true)
                         .Set("string", "potato")
                         .Build();

    EXPECT_EQ(
        "{contexts: [kind: org attributes: {key: string(org-key),  name: "
        "string(Sam) anonymous: bool(false) private: [valid(test)]  custom: "
        "object({{string, string(potato)}, {test, bool(true)}})}, kind: user "
        "attributes: {key: string(user-key),  name: string() anonymous: "
        "bool(false) private: []  custom: object({})}]",
        ProduceString(context_2));
}

TEST(ContextTests, OstreamOperatorInvalidContext) {
    auto context = ContextBuilder().Kind("#$#*(", "").Build();
    EXPECT_FALSE(context.Valid());

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
                                    .Kind("user", "user-key")
                                    .Set("isCat", true)
                                    .SetPrivate("email", "cat@email.email")
                                    .Build());

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
                                    .Kind("user", "user-key")
                                    .Set("isCat", true)
                                    .SetPrivate("email", "cat@email.email")
                                    .Kind("org", "org-key")
                                    .Build());

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
