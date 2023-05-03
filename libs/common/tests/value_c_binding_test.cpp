#include <gtest/gtest.h>

#include "c_bindings/array_builder.h"
#include "c_bindings/object_builder.h"
#include "c_bindings/value.h"

TEST(ValueCBindingTests, CanCreateNull) {
    auto* ptr = LDValue_NewNull();

    EXPECT_EQ(LDValueType_Null, LDValue_Type(ptr));

    LDValue_Free(ptr);
}

TEST(ValueCBindingTests, CanCreateBoolean) {
    auto* true_ptr = LDValue_NewBool(true);

    EXPECT_EQ(LDValueType_Bool, LDValue_Type(true_ptr));

    EXPECT_EQ(true, LDValue_GetBool(true_ptr));

    LDValue_Free(true_ptr);

    auto* false_ptr = LDValue_NewBool(false);

    EXPECT_EQ(LDValueType_Bool, LDValue_Type(false_ptr));

    EXPECT_EQ(false, LDValue_GetBool(false_ptr));

    LDValue_Free(false_ptr);
}

TEST(ValueCBindingTests, CanCreateNumber) {
    auto* ptr = LDValue_NewNumber(17);

    EXPECT_EQ(LDValueType_Number, LDValue_Type(ptr));
    EXPECT_EQ(17, LDValue_GetNumber(ptr));

    LDValue_Free(ptr);
}

TEST(ValueCBindingTests, CanCreateString) {
    auto* ptr = LDValue_NewString("Potato");

    EXPECT_EQ(LDValueType_String, LDValue_Type(ptr));
    EXPECT_EQ(std::string("Potato"), LDValue_GetString(ptr));

    LDValue_Free(ptr);
}

TEST(ValueCBindingTests, CanDuplicateValue) {
    auto* str_val = LDValue_NewString("Potato");
    auto* duplicate = LDValue_NewValue(str_val);
    LDValue_Free(str_val);

    EXPECT_EQ(std::string("Potato"), LDValue_GetString(duplicate));
    EXPECT_EQ(LDValueType_String, LDValue_Type(duplicate));

    LDValue_Free(duplicate);
}

TEST(ValueCBindingTests, CanCreateArray) {
    auto* null_val = LDValue_NewNull();
    auto* bool_val = LDValue_NewBool(true);
    auto* num_val = LDValue_NewNumber(17);
    auto* str_val = LDValue_NewString("Potato");

    auto* array_builder = LDArrayBuilder_New();
    LDArrayBuilder_Add(array_builder, null_val);
    LDArrayBuilder_Add(array_builder, bool_val);
    LDArrayBuilder_Add(array_builder, num_val);
    LDArrayBuilder_Add(array_builder, str_val);

    auto* val_ptr = LDArrayBuilder_Build(array_builder);

    EXPECT_EQ(LDValueType_Array, LDValue_Type(val_ptr));

    auto* iter = LDValue_CreateArrayIter(val_ptr);

    auto index = 0;
    while (!LDValue_ArrayIter_End(iter)) {
        if (index == 0) {
            auto* value_at = LdValue_ArrayIter_Value(iter);
            EXPECT_EQ(LDValueType_Null, LDValue_Type(value_at));
        }
        if (index == 1) {
            auto* value_at = LdValue_ArrayIter_Value(iter);
            EXPECT_EQ(LDValueType_Bool, LDValue_Type(value_at));
            EXPECT_TRUE(LDValue_GetBool(value_at));
        }
        if (index == 2) {
            auto* value_at = LdValue_ArrayIter_Value(iter);
            EXPECT_EQ(LDValueType_Number, LDValue_Type(value_at));
            EXPECT_EQ(17, LDValue_GetNumber(value_at));
        }
        if (index == 3) {
            auto* value_at = LdValue_ArrayIter_Value(iter);
            EXPECT_EQ(LDValueType_String, LDValue_Type(value_at));
            EXPECT_EQ(std::string("Potato"), LDValue_GetString(value_at));
        }
        LDValue_ArrayIter_Next(iter);
        index++;
    }

    LDValue_DestroyArrayIter(iter);

    // Should have iterated 4 items.
    EXPECT_EQ(4, index);

    EXPECT_EQ(4, LDValue_Count(val_ptr));

    LDValue_Free(val_ptr);
}

TEST(ValueCBindingTests, CanCreateObject) {
    auto* null_val = LDValue_NewNull();
    auto* bool_val = LDValue_NewBool(true);
    auto* num_val = LDValue_NewNumber(17);
    auto* str_val = LDValue_NewString("Potato");

    auto* object_builder = LDObjectBuilder_New();
    LDObjectBuilder_Add(object_builder, "null", null_val);
    LDObjectBuilder_Add(object_builder, "bool", bool_val);
    LDObjectBuilder_Add(object_builder, "num", num_val);
    LDObjectBuilder_Add(object_builder, "str", str_val);

    auto* val_ptr = LDObjectBuilder_Build(object_builder);

    EXPECT_EQ(LDValueType_Object, LDValue_Type(val_ptr));

    auto* iter = LDValue_CreateObjectIter(val_ptr);

    auto index = 0;
    while (!LDValue_ObjectIter_End(iter)) {
        auto const* key_at = LdValue_ObjectIter_Key(iter);
        if (strcmp(key_at, "null") == 0) {
            auto* value_at = LdValue_ObjectIter_Value(iter);
            EXPECT_EQ(LDValueType_Null, LDValue_Type(value_at));
            index++;
        }
        if (strcmp(key_at, "bool") == 0) {
            auto* value_at = LdValue_ObjectIter_Value(iter);
            EXPECT_EQ(LDValueType_Bool, LDValue_Type(value_at));
            EXPECT_TRUE(LDValue_GetBool(value_at));
            index++;
        }
        if (strcmp(key_at, "num") == 0) {
            auto* value_at = LdValue_ObjectIter_Value(iter);
            EXPECT_EQ(LDValueType_Number, LDValue_Type(value_at));
            EXPECT_EQ(17, LDValue_GetNumber(value_at));
            index++;
        }
        if (strcmp(key_at, "str") == 0) {
            auto* value_at = LdValue_ObjectIter_Value(iter);
            EXPECT_EQ(LDValueType_String, LDValue_Type(value_at));
            EXPECT_EQ(std::string("Potato"), LDValue_GetString(value_at));
            index++;
        }
        LDValue_ObjectIter_Next(iter);
    }

    LDValue_DestroyObjectIter(iter);

    // Should have iterated 4 items.
    EXPECT_EQ(4, index);

    EXPECT_EQ(4, LDValue_Count(val_ptr));

    LDValue_Free(val_ptr);
}
