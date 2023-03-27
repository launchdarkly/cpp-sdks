#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "value.hpp"

#include <boost/variant.hpp>

using launchdarkly::Value;

TEST(AttributeTests, CanMakeAttributes) {
    //    Value attrStr("test");
    //    Value attrInt(0);

    Value attrDouble(0.0);
    Value attrInt(0);
    Value attrStr(std::string("potato"));
    Value attrBool(false);

    //    Value<std::vector<std::string>> vec =
    //    Value<std::vector<std::string>>(std::vector<std::string>());
}