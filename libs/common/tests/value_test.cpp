#include <gtest/gtest.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "serialization/json_value.hpp"
#include "value.hpp"

#include <boost/json.hpp>

// NOLINTBEGIN cppcoreguidelines-avoid-magic-numbers

using BoostValue = boost::json::value;
using BoostObject = boost::json::object;
using BoostArray = boost::json::array;

using launchdarkly::Value;
TEST(ValueTests, CanMakeNullValue) {
    Value null_val;
    EXPECT_TRUE(null_val.IsNull());
    EXPECT_EQ(Value::Type::kNull, null_val.Type());
}

TEST(ValueTests, CanMoveNullValue) {
    Value null_val(std::move(Value()));
    EXPECT_TRUE(null_val.IsNull());
    EXPECT_EQ(Value::Type::kNull, null_val.Type());
}

TEST(ValueTests, CanMakeBoolValue) {
    Value attr_bool(false);
    EXPECT_TRUE(attr_bool.IsBool());
    EXPECT_FALSE(attr_bool.as_bool());
    EXPECT_EQ(Value::Type::kBool, attr_bool.Type());
}

TEST(ValueTests, CanMoveBoolValue) {
    Value attr_bool(std::move(Value(false)));
    EXPECT_TRUE(attr_bool.IsBool());
    EXPECT_FALSE(attr_bool.as_bool());
    EXPECT_EQ(Value::Type::kBool, attr_bool.Type());
}

TEST(ValueTests, CanMakeDoubleValue) {
    Value attr_double(3.14159);
    EXPECT_TRUE(attr_double.IsNumber());
    EXPECT_EQ(3.14159, attr_double.as_double());
    EXPECT_EQ(3, attr_double.as_int());
    EXPECT_EQ(Value::Type::kNumber, attr_double.Type());
}

TEST(ValueTests, CanMoveDoubleValue) {
    // Not repeated for int, as they use the same data type.
    Value attr_double(std::move(Value(3.14159)));
    EXPECT_TRUE(attr_double.IsNumber());
    EXPECT_EQ(3.14159, attr_double.as_double());
    EXPECT_EQ(3, attr_double.as_int());
    EXPECT_EQ(Value::Type::kNumber, attr_double.Type());
}

TEST(ValueTests, CanMakeIntValue) {
    Value attr_int(42);
    EXPECT_TRUE(attr_int.IsNumber());
    EXPECT_EQ(42, attr_int.as_double());
    EXPECT_EQ(42, attr_int.as_int());
    EXPECT_EQ(Value::Type::kNumber, attr_int.Type());
}

TEST(ValueTests, CanMakeStringValue) {
    Value attr_str(std::string("potato"));
    EXPECT_TRUE(attr_str.IsString());
    EXPECT_EQ("potato", attr_str.as_string());
    EXPECT_EQ(Value::Type::kString, attr_str.Type());
}

TEST(ValueTests, CanMoveStringValue) {
    Value attr_str(std::move(Value(std::string("potato"))));
    EXPECT_TRUE(attr_str.IsString());
    EXPECT_EQ("potato", attr_str.as_string());
    EXPECT_EQ(Value::Type::kString, attr_str.Type());
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

    EXPECT_EQ(Value::Type::kArray, attr_arr.Type());
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

TEST(ValueTests, FromBoostJson) {
    BoostValue bool_val(true);
    auto ld_bool = boost::json::value_to<Value>(bool_val);
    EXPECT_TRUE(ld_bool.as_bool());

    BoostValue string_val("potato");
    auto ld_string = boost::json::value_to<Value>(string_val);
    EXPECT_EQ("potato", ld_string.as_string());

    BoostValue number_val(3.14);
    auto ld_number = boost::json::value_to<Value>(number_val);
    EXPECT_EQ(3.14, ld_number.as_double());

    BoostValue arr_val = BoostArray{true, false, BoostObject{{"name", "Bob"}}};
    auto ld_array = boost::json::value_to<Value>(arr_val);
    EXPECT_TRUE(ld_array.as_array()[0].as_bool());
    EXPECT_FALSE(ld_array.as_array()[1].as_bool());
    EXPECT_EQ("Bob", ld_array.as_array()[2].as_object()["name"].as_string());

    BoostValue obj_val =
        BoostObject{{"name", "Bob"}, {"array", BoostArray{true, false}}};
    auto ld_obj = boost::json::value_to<Value>(obj_val);
    EXPECT_EQ("Bob", ld_obj.as_object()["name"].as_string());
    EXPECT_TRUE(ld_obj.as_object()["array"].as_array()[0].as_bool());
    EXPECT_FALSE(ld_obj.as_object()["array"].as_array()[1].as_bool());
}

TEST(ValueTests, ToBoostJson) {
    Value bool_val(true);
    auto boost_bool = boost::json::value_from(bool_val);
    EXPECT_TRUE(boost_bool.as_bool());

    Value string_val("potato");
    auto boost_string = boost::json::value_from(string_val);
    EXPECT_EQ("potato", boost_string.as_string());

    Value number_val(3.14);
    auto boost_number = boost::json::value_from(number_val);
    EXPECT_EQ(3.14, boost_number.as_double());

    Value arr_val{true, false, {"a", "b"}, Value::Object{{"string", "ham"}}};
    auto boost_arr = boost::json::value_from(arr_val);
    EXPECT_TRUE(boost_arr.as_array().at(0).as_bool());
    EXPECT_FALSE(boost_arr.as_array().at(1).as_bool());
    EXPECT_EQ("a", boost_arr.as_array().at(2).as_array().at(0));
    EXPECT_EQ("b", boost_arr.as_array().at(2).as_array().at(1));
    EXPECT_EQ("ham", boost_arr.as_array().at(3).as_object().at("string"));
}

TEST(ValueTests, ConversionOperators) {
    EXPECT_TRUE(Value(true));
    EXPECT_FALSE(Value(false));
    EXPECT_EQ("potato", static_cast<std::string>(Value("potato")));
    EXPECT_EQ(3.14, static_cast<double>(Value(3.14)));
    EXPECT_EQ(1, static_cast<int>(Value(1)));
}

TEST(ValueTests, ArrayEquality) {
    std::vector<Value::Array> arrays = {
        {"foo", "bar", "baz"},
        {1, 2, 3},
        {1.1, 2.2, 3.3},
        {true, false},
        {true, 1, 3.14, "qux", Value::Array({"foo", "bar"})}};

    for (auto const& a : arrays) {
        ASSERT_TRUE(a == a);
        ASSERT_FALSE(a != a);
    }
}

TEST(ValueTests, ArrayInequality) {
    std::vector<std::pair<Value::Array, Value::Array>> arrays = {
        {{"foo"}, {"bar"}},
        {{"foo"}, {"foo", "foo"}},
        {{1}, {"foo"}},
        {
            {3.14},
            {3},
        },
        {{true}, {false}},
        {{"foo", "bar"}, {"bar", "foo"}}};

    for (auto const& pair : arrays) {
        ASSERT_NE(pair.first, pair.second);
    }
}

TEST(ValueTests, ObjectEqualityOrderDoesNotMatter) {
    std::vector<Value::Object> objects = {
        {{"foo", 1}, {"bar", 2}, {"baz", 3}},
        {{"foo", 1}, {"baz", 3}, {"bar", 2}},
        {{"bar", 2}, {"foo", 1}, {"baz", 3}},
        {{"bar", 2}, {"baz", 3}, {"foo", 1}},
        {{"baz", 3}, {"bar", 2}, {"foo", 1}},
        {{"baz", 3}, {"foo", 1}, {"bar", 2}},
    };

    for (auto const& a : objects) {
        for (auto const& b : objects) {
            ASSERT_TRUE(a == b);
            ASSERT_FALSE(a != b);
        }
    }
}

TEST(ValueTests, ObjectInequality) {
    std::vector<std::pair<Value::Object, Value::Object>> objects = {
        // Different keys, same values
        {Value::Object({{"foo", true}}), Value::Object({{"bar", true}})},
        // Same keys, different values
        {Value::Object({{"foo", true}}), Value::Object({{"foo", false}})},
        // Different number of keys
        {Value::Object({{"foo", true}, {"bar", true}}),
         Value::Object({{"foo", true}})},
        // Same key, but values are arrays with different orderings
        {Value::Object({{"foo", Value({"foo", "bar"})}}),
         Value::Object({{"foo", Value({"bar", "foo"})}})},

    };

    for (auto const& pair : objects) {
        ASSERT_NE(pair.first, pair.second);
    }
}

// NOLINTEND cppcoreguidelines-avoid-magic-numbers
