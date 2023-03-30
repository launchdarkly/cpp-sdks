#include <gtest/gtest.h>

#include "attributes.hpp"

using launchdarkly::AttributeReference;
using launchdarkly::Attributes;
using launchdarkly::Value;

TEST(AttributesTests, CanGetBuiltInAttributesByReference) {
    Attributes attributes("the-key", "the-name", true, Value::null());

    EXPECT_EQ("the-key",
              attributes.get(AttributeReference::from_reference_str("/key"))
                  .as_string());

    EXPECT_EQ("the-name",
              attributes.get(AttributeReference::from_reference_str("/name"))
                  .as_string());

    EXPECT_TRUE(
        attributes.get(AttributeReference::from_reference_str("/anonymous"))
            .as_bool());
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

    EXPECT_EQ(42, attributes.get(AttributeReference::from_reference_str("/int"))
                      .as_int());

    EXPECT_EQ(3.14,
              attributes.get(AttributeReference::from_reference_str("/double"))
                  .as_double());

    EXPECT_EQ("potato",
              attributes.get(AttributeReference::from_reference_str("/string"))
                  .as_string());

    EXPECT_TRUE(attributes.get(AttributeReference::from_reference_str("/bool"))
                    .as_bool());

    EXPECT_TRUE(attributes.get(AttributeReference::from_reference_str("/array"))
                    .as_array()[0]
                    .as_bool());

    EXPECT_FALSE(
        attributes.get(AttributeReference::from_reference_str("/array"))
            .as_array()[1]
            .as_bool());

    EXPECT_EQ("bacon",
              attributes.get(AttributeReference::from_reference_str("/array"))
                  .as_array()[2]
                  .as_string());

    EXPECT_EQ(
        "eggs",
        attributes.get(AttributeReference::from_reference_str("/obj/string"))
            .as_string());
}

TEST(AttributesTests, CanGetSomethingThatDoesNotExist) {
    Attributes attributes("the-key", std::nullopt, true,
                          Value(std::map<std::string, Value>({{"int", 42}})));

    EXPECT_TRUE(
        attributes.get(AttributeReference::from_reference_str("/missing"))
            .is_null());
}

std::string ProduceString(Attributes attrs) {
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

    Attributes attributes2("the-key", "the-name", true,
                           Value(std::map<std::string, Value>({{"int", 42}})),
                           AttributeReference::SetType{"/potato", "/bacon"});

    EXPECT_EQ(
        "{key: string(the-key),  name: string(the-name) anonymous: bool(true) "
        "private: [valid(/bacon), valid(/potato)]  custom: object({{int, "
        "number(42)}})}",
        ProduceString(attributes2));
}
