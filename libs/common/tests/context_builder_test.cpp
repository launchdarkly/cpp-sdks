#include <gtest/gtest.h>

#include "context_builder.hpp"

using launchdarkly::ContextBuilder;

using launchdarkly::Value;
using Object = launchdarkly::Value::Object;
using Array = launchdarkly::Value::Array;
using launchdarkly::AttributeReference;

TEST(ContextBuilderTests, CanMakeBasicContext) {
    auto context = ContextBuilder().kind("user", "user-key").build();

    EXPECT_TRUE(context.valid());

    EXPECT_EQ(1, context.kinds().size());
    EXPECT_EQ("user", context.kinds()[0]);
    EXPECT_EQ("user-key", context.get("user", "/key").as_string());

    EXPECT_EQ("user-key", context.attributes("user").key());
    EXPECT_FALSE(context.attributes("user").anonymous());
}

TEST(ContextBuilderTests, CanMakeSingleContextWithCustomAttributes) {
    auto context = ContextBuilder()
                       .kind("user", "bobby-bobberson")
                       .name("Bob")
                       .anonymous(true)
                       // Set a custom attribute.
                       .set("likesCats", true)
                       // Set a private custom attribute.
                       .set("email", "email@email.email", true)
                       .build();

    EXPECT_TRUE(context.valid());

    EXPECT_EQ("user", context.kinds()[0]);
    EXPECT_EQ("bobby-bobberson", context.get("user", "/key").as_string());
    EXPECT_EQ("Bob", context.get("user", "name").as_string());

    EXPECT_EQ("email@email.email", context.get("user", "email").as_string());
    EXPECT_TRUE(context.get("user", "likesCats").as_bool());

    EXPECT_EQ("bobby-bobberson", context.attributes("user").key());
    EXPECT_TRUE(context.attributes("user").anonymous());
    EXPECT_EQ(1, context.attributes("user").private_attributes().size());
    EXPECT_EQ(1,
              context.attributes("user").private_attributes().count("email"));
}

TEST(ContextBuilderTests, CanBuildComplexMultiContext) {
    auto context =
        ContextBuilder()
            .kind("user", "user-key")
            .anonymous(true)
            .name("test")
            .set("string", "potato")
            .set("int", 42)
            .set("double", 3.14)
            .set("array", {false, true, 42})

            .set("private", "this is private", true)
            .add_private_attribute("double")
            .add_private_attributes(std::vector<std::string>{"string", "int"})
            .add_private_attributes(
                std::vector<AttributeReference>{"explicitArray"})
            // Start the org kind.
            .kind("org", "org-key")
            .name("Macdonwalds")
            .set("explicitArray", Array{"egg", "ham"})
            .set("object", Object{{"string", "bacon"}, {"boolean", false}})
            .build();

    EXPECT_TRUE(context.valid());

    EXPECT_TRUE(context.get("user", "/anonymous").as_bool());
    EXPECT_EQ("test", context.get("user", "/name").as_string());
    EXPECT_EQ("potato", context.get("user", "/string").as_string());
    EXPECT_EQ(42, context.get("user", "int").as_int());
    EXPECT_EQ(3.14, context.get("user", "double").as_double());
    EXPECT_EQ(42, context.get("user", "array").as_array()[2].as_int());
    EXPECT_EQ("ham",
              context.get("org", "explicitArray").as_array()[1].as_string());
    EXPECT_EQ("bacon",
              context.get("org", "object").as_object()["string"].as_string());

    EXPECT_EQ(5, context.attributes("user").private_attributes().size());
    EXPECT_EQ(1, context.attributes("user").private_attributes().count("int"));
    EXPECT_EQ(1,
              context.attributes("user").private_attributes().count("double"));
    EXPECT_EQ(1, context.attributes("user").private_attributes().count(
                     "explicitArray"));
    EXPECT_EQ(1,
              context.attributes("user").private_attributes().count("string"));
    EXPECT_EQ(1,
              context.attributes("user").private_attributes().count("private"));
    EXPECT_EQ("Macdonwalds", context.get("org", "/name").as_string());
}

TEST(ContextBuilderTests, HandlesInvalidKinds) {
    auto context_bad_kind = ContextBuilder().kind("#$#*(", "valid-key").build();
    EXPECT_FALSE(context_bad_kind.valid());

    EXPECT_EQ(
        "#$#*(: \"Kind contained invalid characters. A kind may contain ASCII "
        "letters or numbers, as well as '.', '-', and '_'.\"",
        context_bad_kind.errors());
}

TEST(ContextBuilderTests, HandlesInvalidKeys) {
    auto context_bad_key = ContextBuilder().kind("user", "").build();
    EXPECT_FALSE(context_bad_key.valid());

    EXPECT_EQ("user: \"The key for a context may not be empty.\"",
              context_bad_key.errors());
}

TEST(ContextBuilderTests, HandlesMultipleErrors) {
    auto context = ContextBuilder().kind("#$#*(", "").build();
    EXPECT_FALSE(context.valid());

    EXPECT_EQ(
        "#$#*(: \"Kind contained invalid characters. A kind may contain ASCII "
        "letters or numbers, as well as '.', '-', and '_'.\", #$#*(: \"The key for "
        "a context may not be empty.\"",
        context.errors());
}
