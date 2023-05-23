#include <gtest/gtest.h>

#include <boost/json.hpp>

#include <launchdarkly/attributes.hpp>
#include <launchdarkly/serialization/json_attributes.hpp>

using launchdarkly::AttributeReference;
using launchdarkly::Attributes;
using launchdarkly::Value;

// NOLINTBEGIN cppcoreguidelines-avoid-magic-numbers

TEST(AttributesTests, CanGetBuiltInAttributesByReference) {
    Attributes attributes("the-key", "the-name", true, Value());

    EXPECT_EQ("the-key",
              attributes.Get(AttributeReference::FromReferenceStr("/key"))
                  .AsString());

    EXPECT_EQ("the-name",
              attributes.Get(AttributeReference::FromReferenceStr("/name"))
                  .AsString());

    EXPECT_TRUE(
        attributes.Get(AttributeReference::FromReferenceStr("/anonymous"))
            .AsBool());
}

TEST(AttributesTests, CanGetCustomAttributesByReference) {
    Attributes attributes(
        "the-key", std::nullopt, true,
        Value(std::map<std::string, Value>(
            {{"int", 42},
             {"double", 3.14},
             {"string", "potato"},
             {"bool", true},
             {"array", {true, false, "bacon"}},
             {"obj", std::map<std::string, Value>{{"string", "eggs"}}}})));

    EXPECT_EQ(
        42,
        attributes.Get(AttributeReference::FromReferenceStr("/int")).AsInt());

    EXPECT_EQ(3.14,
              attributes.Get(AttributeReference::FromReferenceStr("/double"))
                  .AsDouble());

    EXPECT_EQ("potato",
              attributes.Get(AttributeReference::FromReferenceStr("/string"))
                  .AsString());

    EXPECT_TRUE(attributes.Get(AttributeReference::FromReferenceStr("/bool"))
                    .AsBool());

    EXPECT_TRUE(attributes.Get(AttributeReference::FromReferenceStr("/array"))
                    .AsArray()[0]
                    .AsBool());

    EXPECT_FALSE(
        attributes.Get(AttributeReference::FromReferenceStr("/array"))
            .AsArray()[1]
            .AsBool());

    EXPECT_EQ("bacon",
              attributes.Get(AttributeReference::FromReferenceStr("/array"))
                  .AsArray()[2]
                  .AsString());

    EXPECT_EQ(
        "eggs",
        attributes.Get(AttributeReference::FromReferenceStr("/obj/string"))
            .AsString());
}

TEST(AttributesTests, CanGetSomethingThatDoesNotExist) {
    Attributes attributes("the-key", std::nullopt, true,
                          Value(std::map<std::string, Value>({{"int", 42}})));

    EXPECT_TRUE(
        attributes.Get(AttributeReference::FromReferenceStr("/missing"))
            .IsNull());
}

std::string ProduceString(Attributes const& attrs) {
    std::stringstream stream;
    stream << attrs;
    stream.flush();
    return stream.str();
}

TEST(AttributesTests, OStreamOperator) {
    Attributes attributes("the-key", std::nullopt, true,
                          Value(std::map<std::string, Value>({{"int", 42}})));
    EXPECT_EQ(
        "{key: string(the-key),  name: null() anonymous: bool(true) private: "
        "[]  custom: object({{int, number(42)}})}",
        ProduceString(attributes));

    Attributes attributes_2("the-key", "the-name", true,
                            Value(std::map<std::string, Value>({{"int", 42}})),
                            AttributeReference::SetType{"/potato", "/bacon"});

    EXPECT_EQ(
        "{key: string(the-key),  name: string(the-name) anonymous: bool(true) "
        "private: [valid(/bacon), valid(/potato)]  custom: object({{int, "
        "number(42)}})}",
        ProduceString(attributes_2));
}

TEST(AttributesTests, JsonSerializationtest) {
    auto attributes_value = boost::json::value_from(
        Attributes("the-key", std::nullopt, true,
                   Value(std::map<std::string, Value>({{"double", 4.2}})),
                   AttributeReference::SetType{"/potato", "/bacon"}));

    auto parsed_value = boost::json::parse(
        "{"
        "\"key\":\"the-key\","
        "\"anonymous\":true,"
        "\"double\":4.2,"
        "\"_meta\":{\"privateAttributes\": [\"/bacon\", \"/potato\"]}"
        "}");

    EXPECT_EQ(parsed_value, attributes_value);
}

TEST(AttributesTests, JsonSerializationOmitsFalseAnonymous) {
    auto attributes_value = boost::json::value_from(
        Attributes("the-key", std::nullopt, false,
                   Value(std::map<std::string, Value>({{"double", 4.2}})),
                   AttributeReference::SetType{"/potato", "/bacon"}));

    auto parsed_value = boost::json::parse(
        "{"
        "\"key\":\"the-key\","
        "\"double\":4.2,"
        "\"_meta\":{\"privateAttributes\": [\"/bacon\", \"/potato\"]}"
        "}");

    EXPECT_EQ(parsed_value, attributes_value);
}

TEST(AttributesTests, JsonSerializationOmitsMetaIfPrivateAttributesEmpty) {
    auto attributes_value = boost::json::value_from(
        Attributes("the-key", std::nullopt, false,
                   Value(std::map<std::string, Value>({{"double", 4.2}}))));

    auto parsed_value = boost::json::parse(
        "{"
        "\"key\":\"the-key\","
        "\"double\":4.2"
        "}");

    EXPECT_EQ(parsed_value, attributes_value);
}

// NOLINTEND cppcoreguidelines-avoid-magic-numbers
