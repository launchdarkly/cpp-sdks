#include <gtest/gtest.h>

#include <map>
#include <vector>

#include <launchdarkly/context_builder.hpp>

using launchdarkly::ContextBuilder;

using launchdarkly::Value;
using Object = launchdarkly::Value::Object;
using Array = launchdarkly::Value::Array;
using launchdarkly::AttributeReference;

// NOLINTBEGIN cppcoreguidelines-avoid-magic-numbers

TEST(ContextBuilderTests, CanMakeBasicContext) {
    auto context = ContextBuilder().Kind("user", "user-key").Build();

    EXPECT_TRUE(context.Valid());

    EXPECT_EQ(1, context.Kinds().size());
    EXPECT_EQ("user", context.Kinds()[0]);
    EXPECT_EQ("user-key", context.Get("user", "/key").AsString());

    EXPECT_EQ("user-key", context.Attributes("user").Key());
    EXPECT_FALSE(context.Attributes("user").Anonymous());
}

TEST(ContextBuilderTests, CanMakeSingleContextWithCustomAttributes) {
    auto context = ContextBuilder()
                       .Kind("user", "bobby-bobberson")
                       .Name("Bob")
                       .Anonymous(true)
                       // Set a custom attribute.
                       .Set("likesCats", true)
                       // Set a private custom attribute.
                       .SetPrivate("email", "email@email.email")
                       .Build();

    EXPECT_TRUE(context.Valid());

    EXPECT_EQ("user", context.Kinds()[0]);
    EXPECT_EQ("bobby-bobberson", context.Get("user", "/key").AsString());
    EXPECT_EQ("Bob", context.Get("user", "name").AsString());

    EXPECT_EQ("email@email.email", context.Get("user", "email").AsString());
    EXPECT_TRUE(context.Get("user", "likesCats").AsBool());

    EXPECT_EQ("bobby-bobberson", context.Attributes("user").Key());
    EXPECT_TRUE(context.Attributes("user").Anonymous());
    EXPECT_EQ(1, context.Attributes("user").PrivateAttributes().size());
    EXPECT_EQ(1, context.Attributes("user").PrivateAttributes().count("email"));
}

TEST(ContextBuilderTests, CanBuildComplexMultiContext) {
    auto context =
        ContextBuilder()
            .Kind("user", "user-key")
            .Anonymous(true)
            .Name("test")
            .Set("string", "potato")
            .Set("int", 42)
            .Set("double", 3.14)
            .Set("array", {false, true, 42})

            .SetPrivate("private", "this is private")
            .AddPrivateAttribute("double")
            .AddPrivateAttributes(std::vector<std::string>{"string", "int"})
            .AddPrivateAttributes(
                std::vector<AttributeReference>{"explicitArray"})
            // Start the org kind.
            .Kind("org", "org-key")
            .Name("Macdonwalds")
            .Set("explicitArray", Array{"egg", "ham"})
            .Set("object", Object{{"string", "bacon"}, {"boolean", false}})
            .Build();

    EXPECT_TRUE(context.Valid());

    EXPECT_TRUE(context.Get("user", "/anonymous").AsBool());
    EXPECT_EQ("test", context.Get("user", "/name").AsString());
    EXPECT_EQ("potato", context.Get("user", "/string").AsString());
    EXPECT_EQ(42, context.Get("user", "int").AsInt());
    EXPECT_EQ(3.14, context.Get("user", "double").AsDouble());
    EXPECT_EQ(42, context.Get("user", "array").AsArray()[2].AsInt());
    EXPECT_EQ("ham",
              context.Get("org", "explicitArray").AsArray()[1].AsString());
    EXPECT_EQ("bacon",
              context.Get("org", "object").AsObject()["string"].AsString());

    EXPECT_EQ(5, context.Attributes("user").PrivateAttributes().size());
    EXPECT_EQ(1, context.Attributes("user").PrivateAttributes().count("int"));
    EXPECT_EQ(1,
              context.Attributes("user").PrivateAttributes().count("double"));
    EXPECT_EQ(1, context.Attributes("user").PrivateAttributes().count(
                     "explicitArray"));
    EXPECT_EQ(1,
              context.Attributes("user").PrivateAttributes().count("string"));
    EXPECT_EQ(1,
              context.Attributes("user").PrivateAttributes().count("private"));
    EXPECT_EQ("Macdonwalds", context.Get("org", "/name").AsString());
}

TEST(ContextBuilderTests, HandlesInvalidKinds) {
    auto context_bad_kind = ContextBuilder().Kind("#$#*(", "valid-key").Build();
    EXPECT_FALSE(context_bad_kind.Valid());

    EXPECT_EQ(
        "#$#*(: \"Kind contained invalid characters. A kind may contain ASCII "
        "letters or numbers, as well as '.', '-', and '_'.\"",
        context_bad_kind.errors());
}

TEST(ContextBuilderTests, HandlesInvalidKeys) {
    auto context_bad_key = ContextBuilder().Kind("user", "").Build();
    EXPECT_FALSE(context_bad_key.Valid());

    EXPECT_EQ("user: \"The key for a context may not be empty.\"",
              context_bad_key.errors());
}

TEST(ContextBuilderTests, HandlesMultipleErrors) {
    auto context = ContextBuilder().Kind("#$#*(", "").Build();
    EXPECT_FALSE(context.Valid());

    EXPECT_EQ(
        "#$#*(: \"Kind contained invalid characters. A kind may contain ASCII "
        "letters or numbers, as well as '.', '-', and '_'.\", #$#*(: \"The key "
        "for "
        "a context may not be empty.\"",
        context.errors());
}

TEST(ContextBuilderTests, HandlesEmptyContext) {
    auto context = ContextBuilder().Build();
    EXPECT_FALSE(context.Valid());
    EXPECT_EQ("\"The context must contain at least 1 kind.\"",
              context.errors());
}

TEST(ContextBuilderTests, UseWithLoops) {
    auto kinds = std::vector<std::string>{"user", "org"};
    auto props = std::map<std::string, Value>{{"a", "b"}, {"c", "d"}};

    auto builder = ContextBuilder();

    for (auto const& kind : kinds) {
        auto& kind_builder = builder.Kind(kind, kind + "-key");
        for (auto const& prop : props) {
            kind_builder.Set(prop.first, prop.second);
        }
    }

    auto context = builder.Build();

    EXPECT_EQ("b", context.Get("user", "/a").AsString());
    EXPECT_EQ("d", context.Get("user", "/c").AsString());

    EXPECT_EQ("b", context.Get("org", "/a").AsString());
    EXPECT_EQ("d", context.Get("org", "/c").AsString());
}

TEST(ContextBuilderTests, AccessKindBuilderMultipleTimes) {
    auto builder = ContextBuilder();

    builder.Kind("user", "potato").Name("Bob").Set("city", "Reno");
    builder.Kind("user", "ham").Set("isCat", true);

    auto context = builder.Build();

    EXPECT_EQ("ham", context.Get("user", "key").AsString());
    EXPECT_EQ("Bob", context.Get("user", "name").AsString());
    EXPECT_EQ("Reno", context.Get("user", "city").AsString());
    EXPECT_TRUE(context.Get("user", "isCat").AsBool());
}

TEST(ContextBuilderTests, AddAttributeToExistingContext) {
    auto context = ContextBuilder()
                       .Kind("user", "potato")
                       .Name("Bob")
                       .Set("city", "Reno")
                       .SetPrivate("private", "a")
                       .Build();

    auto builder = ContextBuilder(context);
    if (auto updater = builder.Update("user")) {
        updater->Set("state", "Nevada");
        updater->SetPrivate("sneaky", true);
    }
    auto updated_context = builder.Build();

    EXPECT_EQ("potato", updated_context.Get("user", "key").AsString());
    EXPECT_EQ("Bob", updated_context.Get("user", "name").AsString());
    EXPECT_EQ("Reno", updated_context.Get("user", "city").AsString());
    EXPECT_EQ("Nevada", updated_context.Get("user", "state").AsString());

    EXPECT_EQ(2, updated_context.Attributes("user").PrivateAttributes().size());
    EXPECT_EQ(1, updated_context.Attributes("user").PrivateAttributes().count("private"));
    EXPECT_EQ(1, updated_context.Attributes("user").PrivateAttributes().count("sneaky"));
}

TEST(ContextBuilderTests, AddKindToExistingContext) {
    auto context = ContextBuilder()
                       .Kind("user", "potato")
                       .Name("Bob")
                       .Set("city", "Reno")
                       .Build();

    auto updated_context =
        ContextBuilder(context).Kind("org", "org-key").Build();

    EXPECT_EQ("potato", updated_context.Get("user", "key").AsString());
    EXPECT_EQ("Bob", updated_context.Get("user", "name").AsString());
    EXPECT_EQ("Reno", updated_context.Get("user", "city").AsString());

    EXPECT_EQ("org-key", updated_context.Get("org", "key"));
}

TEST(ContextBuilderTests, UseTheSameBuilderToBuildMultipleContexts) {
    auto builder = ContextBuilder();

    auto context_a =
        builder.Kind("user", "potato").Name("Bob").Set("city", "Reno").Build();

    auto context_b = builder.Build();

    auto context_c = builder.Kind("bob", "tomato").Build();

    EXPECT_EQ("potato", context_a.Get("user", "key").AsString());
    EXPECT_EQ("Bob", context_a.Get("user", "name").AsString());
    EXPECT_EQ("Reno", context_a.Get("user", "city").AsString());

    EXPECT_EQ("potato", context_b.Get("user", "key").AsString());
    EXPECT_EQ("Bob", context_b.Get("user", "name").AsString());
    EXPECT_EQ("Reno", context_b.Get("user", "city").AsString());

    EXPECT_EQ("potato", context_c.Get("user", "key").AsString());
    EXPECT_EQ("Bob", context_c.Get("user", "name").AsString());
    EXPECT_EQ("Reno", context_c.Get("user", "city").AsString());
    EXPECT_EQ("tomato", context_c.Get("bob", "key").AsString());
}

// NOLINTEND cppcoreguidelines-avoid-magic-numbers
