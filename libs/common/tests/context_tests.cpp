#include <gtest/gtest.h>

#include "context_builder.hpp"

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
    EXPECT_EQ(2, context.keys_and_kinds().size());
    EXPECT_EQ("user-key", context.keys_and_kinds().find("user")->second);
    EXPECT_EQ("org-key", context.keys_and_kinds().find("org")->second);
}

std::string ProduceString(Context ctx) {
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

    auto context2 = ContextBuilder()
                        .kind("user", "user-key")
                        .kind("org", "org-key")
                        .name("Sam")
                        .set("test", true, true)
                        .set("string", "potato")
                        .build();

    EXPECT_EQ(
        "{contexts: [kind: org attributes: {key: string(org-key),  name: "
        "string(Sam) anonymous: bool(false) private: [valid(test)]  custom: "
        "object({{string, string(potato)}, {test, bool(true)}})}, kind: user "
        "attributes: {key: string(user-key),  name: string() anonymous: "
        "bool(false) private: []  custom: object({})}]",
        ProduceString(context2));
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
