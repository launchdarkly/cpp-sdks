#include <gtest/gtest.h>

#include "launchdarkly/bindings/c/array_builder.h"
#include "launchdarkly/bindings/c/object_builder.h"
#include "launchdarkly/bindings/c/value.h"

#include "launchdarkly/bindings/c/memory_routines.h"

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

    auto* iter = LDValue_ArrayIter_New(val_ptr);

    auto index = 0;
    while (!LDValue_ArrayIter_End(iter)) {
        if (index == 0) {
            auto* value_at = LDValue_ArrayIter_Value(iter);
            EXPECT_EQ(LDValueType_Null, LDValue_Type(value_at));
        }
        if (index == 1) {
            auto* value_at = LDValue_ArrayIter_Value(iter);
            EXPECT_EQ(LDValueType_Bool, LDValue_Type(value_at));
            EXPECT_TRUE(LDValue_GetBool(value_at));
        }
        if (index == 2) {
            auto* value_at = LDValue_ArrayIter_Value(iter);
            EXPECT_EQ(LDValueType_Number, LDValue_Type(value_at));
            EXPECT_EQ(17, LDValue_GetNumber(value_at));
        }
        if (index == 3) {
            auto* value_at = LDValue_ArrayIter_Value(iter);
            EXPECT_EQ(LDValueType_String, LDValue_Type(value_at));
            EXPECT_EQ(std::string("Potato"), LDValue_GetString(value_at));
        }
        LDValue_ArrayIter_Next(iter);
        index++;
    }

    LDValue_ArrayIter_Free(iter);

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

    auto* iter = LDValue_ObjectIter_New(val_ptr);

    auto index = 0;
    while (!LDValue_ObjectIter_End(iter)) {
        auto const* key_at = LDValue_ObjectIter_Key(iter);
        if (strcmp(key_at, "null") == 0) {
            auto* value_at = LDValue_ObjectIter_Value(iter);
            EXPECT_EQ(LDValueType_Null, LDValue_Type(value_at));
            index++;
        }
        if (strcmp(key_at, "bool") == 0) {
            auto* value_at = LDValue_ObjectIter_Value(iter);
            EXPECT_EQ(LDValueType_Bool, LDValue_Type(value_at));
            EXPECT_TRUE(LDValue_GetBool(value_at));
            index++;
        }
        if (strcmp(key_at, "num") == 0) {
            auto* value_at = LDValue_ObjectIter_Value(iter);
            EXPECT_EQ(LDValueType_Number, LDValue_Type(value_at));
            EXPECT_EQ(17, LDValue_GetNumber(value_at));
            index++;
        }
        if (strcmp(key_at, "str") == 0) {
            auto* value_at = LDValue_ObjectIter_Value(iter);
            EXPECT_EQ(LDValueType_String, LDValue_Type(value_at));
            EXPECT_EQ(std::string("Potato"), LDValue_GetString(value_at));
            index++;
        }
        LDValue_ObjectIter_Next(iter);
    }

    LDValue_ObjectIter_Free(iter);

    // Should have iterated 4 items.
    EXPECT_EQ(4, index);

    EXPECT_EQ(4, LDValue_Count(val_ptr));

    LDValue_Free(val_ptr);
}

// Helper to serialize an LDValue, automatically converts to
// std::string and frees the result using LDMemory_FreeString.
std::string serialize(LDValue const val) {
    char* serialized = LDValue_SerializeJSON(val);
    std::string result(serialized);
    LDMemory_FreeString(serialized);
    return result;
}

TEST(ValueCBindingTests, CanSerializeToJSON) {
    auto* null_val = LDValue_NewNull();
    auto* bool_val_true = LDValue_NewBool(true);
    auto* bool_val_false = LDValue_NewBool(false);

    auto* num_val = LDValue_NewNumber(17);
    auto* float_val = LDValue_NewNumber(3.141);
    auto* str_val = LDValue_NewString("Potato");

    EXPECT_EQ("null", serialize(null_val));
    EXPECT_EQ("true", serialize(bool_val_true));
    EXPECT_EQ("false", serialize(bool_val_false));
    EXPECT_EQ("1.7E1", serialize(num_val));
    EXPECT_EQ("3.141E0", serialize(float_val));
    EXPECT_EQ("\"Potato\"", serialize(str_val));

    // Object builder is going to take care of freeing all the primitives
    // (except for bool_val_false.)
    auto* object_builder = LDObjectBuilder_New();
    LDObjectBuilder_Add(object_builder, "null", null_val);
    LDObjectBuilder_Add(object_builder, "bool", bool_val_true);
    LDObjectBuilder_Add(object_builder, "num", num_val);
    LDObjectBuilder_Add(object_builder, "float", float_val);
    LDObjectBuilder_Add(object_builder, "str", str_val);

    auto* obj_ptr = LDObjectBuilder_Build(object_builder);

    EXPECT_EQ(
        "{\"bool\":true,\"float\":3.141E0,\"null\":null,\"num\":1.7E1,\"str\":"
        "\"Potato\"}",
        serialize(obj_ptr));

    LDValue_Free(obj_ptr);

    // Array builder is going to take care of freeing bool_val_false.
    auto* array_builder = LDArrayBuilder_New();
    LDArrayBuilder_Add(array_builder, bool_val_false);
    auto* array_ptr = LDArrayBuilder_Build(array_builder);

    EXPECT_EQ("[false]", serialize(array_ptr));

    LDValue_Free(array_ptr);
}
