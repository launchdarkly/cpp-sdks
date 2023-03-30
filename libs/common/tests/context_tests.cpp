#include <gtest/gtest.h>

#include "context_builder.hpp"

using launchdarkly::Context;
using launchdarkly::ContextBuilder;

std::string ProduceString(Context ctx) {
    std::stringstream stream;
    stream << ctx;
    stream.flush();
    return stream.str();
}

TEST(ContextTexts, OstreamOperator) {
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
