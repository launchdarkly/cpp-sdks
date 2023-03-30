#include <gtest/gtest.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "value.hpp"

// NOLINTBEGIN cppcoreguidelines-avoid-magic-numbers

using launchdarkly::Value;
TEST(ValueTests, CanMakeNullValue) {
    Value null_val;
    EXPECT_TRUE(null_val.is_null());
    EXPECT_EQ(Value::Type::kNull, null_val.type());
}

TEST(ValueTests, CanMoveNullValue) {
    Value null_val(std::move(Value()));
    EXPECT_TRUE(null_val.is_null());
    EXPECT_EQ(Value::Type::kNull, null_val.type());
}

TEST(ValueTests, CanMakeBoolValue) {
    Value attr_bool(false);
    EXPECT_TRUE(attr_bool.is_bool());
    EXPECT_FALSE(attr_bool.as_bool());
    EXPECT_EQ(Value::Type::kBool, attr_bool.type());
}

TEST(ValueTests, CanMoveBoolValue) {
    Value attr_bool(std::move(Value(false)));
    EXPECT_TRUE(attr_bool.is_bool());
    EXPECT_FALSE(attr_bool.as_bool());
    EXPECT_EQ(Value::Type::kBool, attr_bool.type());
}

TEST(ValueTests, CanMakeDoubleValue) {
    Value attr_double(3.14159);
    EXPECT_TRUE(attr_double.is_number());
    EXPECT_EQ(3.14159, attr_double.as_double());
    EXPECT_EQ(3, attr_double.as_int());
    EXPECT_EQ(Value::Type::kNumber, attr_double.type());
}

TEST(ValueTests, CanMoveDoubleValue) {
    // Not repeated for int, as they use the same data type.
    Value attr_double(std::move(Value(3.14159)));
    EXPECT_TRUE(attr_double.is_number());
    EXPECT_EQ(3.14159, attr_double.as_double());
    EXPECT_EQ(3, attr_double.as_int());
    EXPECT_EQ(Value::Type::kNumber, attr_double.type());
}

TEST(ValueTests, CanMakeIntValue) {
    Value attr_int(42);
    EXPECT_TRUE(attr_int.is_number());
    EXPECT_EQ(42, attr_int.as_double());
    EXPECT_EQ(42, attr_int.as_int());
    EXPECT_EQ(Value::Type::kNumber, attr_int.type());
}

TEST(ValueTests, CanMakeStringValue) {
    Value attr_str(std::string("potato"));
    EXPECT_TRUE(attr_str.is_string());
    EXPECT_EQ("potato", attr_str.as_string());
    EXPECT_EQ(Value::Type::kString, attr_str.type());
}

TEST(ValueTests, CanMoveStringValue) {
    Value attr_str(std::move(Value(std::string("potato"))));
    EXPECT_TRUE(attr_str.is_string());
    EXPECT_EQ("potato", attr_str.as_string());
    EXPECT_EQ(Value::Type::kString, attr_str.type());
}

void VectorAssertions(Value attr_arr) {
    EXPECT_TRUE(attr_arr.is_array());
    EXPECT_FALSE(attr_arr.as_array()[0].as_bool());
    EXPECT_TRUE(attr_arr.as_array()[1].as_bool());
    EXPECT_EQ("potato", attr_arr.as_array()[2].as_string());
    EXPECT_EQ(42, attr_arr.as_array()[3].as_int());
    EXPECT_EQ(3.14, attr_arr.as_array()[4].as_double());

    EXPECT_EQ("a", attr_arr.as_array()[5].as_array()[0].as_string());
    EXPECT_EQ(21, attr_arr.as_array()[5].as_array()[1].as_int());

    EXPECT_EQ(
        "bacon",
        attr_arr.as_array()[6].as_object().find("string")->second.as_string());

    EXPECT_EQ(Value::Type::kArray, attr_arr.type());
}

TEST(ValueTests, CanMakeFromVector) {
    Value attr_arr(std::vector<Value>{
        false, true, "potato", 42, 3.14, std::vector<Value>{"a", 21},
        std::map<std::string, Value>{{"string", "bacon"}}});

    VectorAssertions(attr_arr);
}

TEST(ValueTests, CanMoveVectorValue) {
    Value attr_arr(std::move(Value(std::vector<Value>{
        false, true, "potato", 42, 3.14, std::vector<Value>{"a", 21},
        std::map<std::string, Value>{{"string", "bacon"}}})));
    VectorAssertions(attr_arr);
}

TEST(ValueTests, CanMakeFromInitListVector) {
    Value init_list = {false,
                       true,
                       "potato",
                       42,
                       3.14,
                       {"a", 21},
                       std::map<std::string, Value>{{"string", "bacon"}}};

    VectorAssertions(init_list);
}

void MapAssertions(Value const& attr_map) {
    EXPECT_TRUE(attr_map.is_object());
    EXPECT_TRUE(attr_map.as_object()["bool"].as_bool());

    EXPECT_TRUE(attr_map.is_object());
    // Can index.
    EXPECT_EQ(3.14, attr_map.as_object()["double"].as_double());
    // Can use find.
    EXPECT_EQ(42, attr_map.as_object().find("int")->second.as_int());
    EXPECT_EQ("potato",
              attr_map.as_object().find("string")->second.as_string());
    EXPECT_TRUE(
        attr_map.as_object().find("array")->second.as_array()[0].as_bool());
    EXPECT_FALSE(
        attr_map.as_object().find("array")->second.as_array()[1].as_bool());
    EXPECT_EQ(
        "bacon",
        attr_map.as_object().find("array")->second.as_array()[2].as_string());

    EXPECT_EQ("eggs", attr_map.as_object()
                          .find("obj")
                          ->second.as_object()
                          .find("string")
                          ->second.as_string());
}

TEST(ValueTests, CanMakeFromMap) {
    Value attr_map(std::map<std::string, Value>(
        {{"int", 42},
         {"double", 3.14},
         {"string", "potato"},
         {"bool", true},
         {"array", {true, false, "bacon"}},
         {"obj", std::map<std::string, Value>{{"string", "eggs"}}}}));

    MapAssertions(attr_map);
}

TEST(ValueTests, CanMoveMap) {
    Value attr_map(std::move(Value(std::map<std::string, Value>(
        {{"int", 42},
         {"double", 3.14},
         {"string", "potato"},
         {"bool", true},
         {"array", {true, false, "bacon"}},
         {"obj", std::map<std::string, Value>{{"string", "eggs"}}}}))));

    MapAssertions(attr_map);
}

TEST(ValueTests, AssignToExistingValue) {
    // Running this test with valgrind should being to light any issues with
    // cleaning things up during moves and copies.
    auto str_value = Value("test");
    EXPECT_EQ("test", str_value.as_string());
    auto second_str = Value("second");
    str_value = std::move(second_str);
    // Move assignment.
    EXPECT_EQ("second", str_value.as_string());

    auto third_str = Value("third");
    // Copy assignment
    str_value = third_str;
    EXPECT_EQ("third", str_value.as_string());
}

std::string ProduceString(Value val) {
    std::stringstream stream;
    stream << val;
    stream.flush();
    return stream.str();
}

TEST(ValueTests, OstreamOperator) {
    EXPECT_EQ("null()", ProduceString(Value()));

    EXPECT_EQ("bool(false)", ProduceString(Value(false)));
    EXPECT_EQ("bool(true)", ProduceString(Value(true)));

    EXPECT_EQ("number(3.14)", ProduceString(Value(3.14)));
    EXPECT_EQ("number(42)", ProduceString(Value(42)));

    EXPECT_EQ("string(potato)", ProduceString(Value("potato")));

    EXPECT_EQ("string(potato)", ProduceString(Value("potato")));

    EXPECT_EQ("array([string(ham), string(cheese)])",
              ProduceString(Value{"ham", "cheese"}));

    EXPECT_EQ(
        "object({{first, string(cheese)}, {second, number(42)}})",
        ProduceString(Value::Object{{"first", "cheese"}, {"second", 42}}));
}

// NOLINTEND cppcoreguidelines-avoid-magic-numbers
