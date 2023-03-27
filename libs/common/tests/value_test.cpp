#include <gtest/gtest.h>
#include <map>
#include <string>
#include <vector>

#include "value.hpp"

using launchdarkly::Value;

TEST(ValueTests, CanMakeNullValue) {
    Value nullVal;
    EXPECT_TRUE(nullVal.is_null());
    EXPECT_EQ(Value::Type::kNull, nullVal.type());
}

TEST(ValueTests, CanMoveNullValue) {
    Value nullVal(std::move(Value()));
    EXPECT_TRUE(nullVal.is_null());
    EXPECT_EQ(Value::Type::kNull, nullVal.type());
}

TEST(ValueTests, CanMakeBoolValue) {
    Value attrBool(false);
    EXPECT_TRUE(attrBool.is_bool());
    EXPECT_FALSE(attrBool.as_bool());
    EXPECT_EQ(Value::Type::kBool, attrBool.type());
}

TEST(ValueTests, CanMoveBoolValue) {
    Value attrBool(std::move(Value(false)));
    EXPECT_TRUE(attrBool.is_bool());
    EXPECT_FALSE(attrBool.as_bool());
    EXPECT_EQ(Value::Type::kBool, attrBool.type());
}

TEST(ValueTests, CanMakeDoubleValue) {
    Value attrDouble(3.14159);
    EXPECT_TRUE(attrDouble.is_number());
    EXPECT_EQ(3.14159, attrDouble.as_double());
    EXPECT_EQ(3, attrDouble.as_int());
    EXPECT_EQ(Value::Type::kNumber, attrDouble.type());
}

TEST(ValueTests, CanMoveDoubleValue) {
    // Not repeated for int, as they use the same data type.
    Value attrDouble(std::move(Value(3.14159)));
    EXPECT_TRUE(attrDouble.is_number());
    EXPECT_EQ(3.14159, attrDouble.as_double());
    EXPECT_EQ(3, attrDouble.as_int());
    EXPECT_EQ(Value::Type::kNumber, attrDouble.type());
}

TEST(ValueTests, CanMakeIntValue) {
    Value attrInt(42);
    EXPECT_TRUE(attrInt.is_number());
    EXPECT_EQ(42, attrInt.as_double());
    EXPECT_EQ(42, attrInt.as_int());
    EXPECT_EQ(Value::Type::kNumber, attrInt.type());
}

TEST(ValueTests, CanMakeStringValue) {
    Value attrStr(std::string("potato"));
    EXPECT_TRUE(attrStr.is_string());
    EXPECT_EQ("potato", attrStr.as_string());
    EXPECT_EQ(Value::Type::kString, attrStr.type());
}

TEST(ValueTests, CanMoveStringValue) {
    Value attrStr(std::move(Value(std::string("potato"))));
    EXPECT_TRUE(attrStr.is_string());
    EXPECT_EQ("potato", attrStr.as_string());
    EXPECT_EQ(Value::Type::kString, attrStr.type());
}

void vector_assertions(Value attrArr) {
    EXPECT_TRUE(attrArr.is_array());
    EXPECT_FALSE(attrArr.as_array()[0].as_bool());
    EXPECT_TRUE(attrArr.as_array()[1].as_bool());
    EXPECT_EQ("potato", attrArr.as_array()[2].as_string());
    EXPECT_EQ(42, attrArr.as_array()[3].as_int());
    EXPECT_EQ(3.14, attrArr.as_array()[4].as_double());

    EXPECT_EQ("a", attrArr.as_array()[5].as_array()[0].as_string());
    EXPECT_EQ(21, attrArr.as_array()[5].as_array()[1].as_int());

    EXPECT_EQ(
        "bacon",
        attrArr.as_array()[6].as_object().find("string")->second.as_string());

    EXPECT_EQ(Value::Type::kArray, attrArr.type());
}

TEST(ValueTests, CanMakeFromVector) {
    Value attrArr(std::vector<Value>{
        false, true, "potato", 42, 3.14, std::vector<Value>{"a", 21},
        std::map<std::string, Value>{{"string", "bacon"}}});

    vector_assertions(attrArr);
}

TEST(ValueTests, CanMoveVectorValue) {
    Value attrArr(std::move(Value(std::vector<Value>{
        false, true, "potato", 42, 3.14, std::vector<Value>{"a", 21},
        std::map<std::string, Value>{{"string", "bacon"}}})));
    vector_assertions(attrArr);
}

TEST(ValueTests, CanMakeFromInitListVector) {
    Value initList = {false,
                      true,
                      "potato",
                      42,
                      3.14,
                      {"a", 21},
                      std::map<std::string, Value>{{"string", "bacon"}}};
}

void map_assertions(Value const& attrMap) {
    EXPECT_TRUE(attrMap.is_object());
    EXPECT_TRUE(attrMap.as_object().find("bool")->second.as_bool());

    EXPECT_TRUE(attrMap.is_object());
    EXPECT_EQ(3.14, attrMap.as_object().find("double")->second.as_double());
    EXPECT_EQ(42, attrMap.as_object().find("int")->second.as_int());
    EXPECT_EQ("potato", attrMap.as_object().find("string")->second.as_string());
    EXPECT_TRUE(
        attrMap.as_object().find("array")->second.as_array()[0].as_bool());
    EXPECT_FALSE(
        attrMap.as_object().find("array")->second.as_array()[1].as_bool());
    EXPECT_EQ(
        "bacon",
        attrMap.as_object().find("array")->second.as_array()[2].as_string());

    EXPECT_EQ("eggs", attrMap.as_object()
                          .find("obj")
                          ->second.as_object()
                          .find("string")
                          ->second.as_string());
}

TEST(ValueTests, CanMakeFromMap) {
    Value attrMap(std::map<std::string, Value>(
        {{"int", 42},
         {"double", 3.14},
         {"string", "potato"},
         {"bool", true},
         {"array", {true, false, "bacon"}},
         {"obj", std::map<std::string, Value>{{"string", "eggs"}}}}));

    map_assertions(attrMap);
}

TEST(ValueTests, CanMoveMap) {
    Value attrMap(std::move(Value(std::map<std::string, Value>(
        {{"int", 42},
         {"double", 3.14},
         {"string", "potato"},
         {"bool", true},
         {"array", {true, false, "bacon"}},
         {"obj", std::map<std::string, Value>{{"string", "eggs"}}}}))));

    map_assertions(attrMap);
}

TEST(ValueTests, AssignToExistingValue) {
    // Running this test with valgrind should being to light any issues with
    // cleaning things up during moves and copies.
    auto strValue = Value("test");
    EXPECT_EQ("test", strValue.as_string());
    auto secondStr = Value("second");
    strValue = std::move(secondStr);
    //Move assignment.
    EXPECT_EQ("second", strValue.as_string());

    auto thirdValue = Value("third");
    // Copy assignment
    strValue = thirdValue;
    EXPECT_EQ("third", strValue.as_string());
}