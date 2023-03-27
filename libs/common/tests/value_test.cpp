#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "value.hpp"

#include <boost/variant.hpp>

using launchdarkly::Value;

TEST(ValueTests, CanMakeBasicAttributes) {
    Value nullVal;
    EXPECT_TRUE(nullVal.is_null());
    EXPECT_EQ(Value::Type::kNull, nullVal.type());
    EXPECT_EQ(Value::Type::kNull, nullVal.type());

    Value attrBool(false);
    EXPECT_TRUE(attrBool.is_bool());
    EXPECT_FALSE(attrBool.as_bool());
    EXPECT_EQ(Value::Type::kBool, attrBool.type());

    Value attrDouble(3.14159);
    EXPECT_TRUE(attrDouble.is_number());
    EXPECT_EQ(3.14159, attrDouble.as_double());
    EXPECT_EQ(3, attrDouble.as_int());
    EXPECT_EQ(Value::Type::kNumber, attrDouble.type());

    Value attrInt(42);
    EXPECT_TRUE(attrInt.is_number());
    EXPECT_EQ(42, attrInt.as_double());
    EXPECT_EQ(42, attrInt.as_int());
    EXPECT_EQ(Value::Type::kNumber, attrDouble.type());

    Value attrStr(std::string("potato"));
    EXPECT_TRUE(attrStr.is_string());
    EXPECT_EQ("potato", attrStr.as_string());
    EXPECT_EQ(Value::Type::kString, attrStr.type());

    Value attrArr(std::vector<Value>{false, true, "potato", 42, 3.14});
    EXPECT_TRUE(attrArr.is_array());
    EXPECT_FALSE(attrArr.as_array()[0].as_bool());
    EXPECT_TRUE(attrArr.as_array()[1].as_bool());
    EXPECT_EQ("potato", attrArr.as_array()[2].as_string());
    EXPECT_EQ(42, attrArr.as_array()[3].as_int());
    EXPECT_EQ(3.14, attrArr.as_array()[4].as_double());
    EXPECT_EQ(Value::Type::kArray, attrArr.type());

    //    Value attrObj(std::map<std::string, Value>{{"potato", "bacon"}});
}