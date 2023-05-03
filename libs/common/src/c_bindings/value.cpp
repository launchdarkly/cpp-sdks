// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include "c_bindings/value.h"
#include "value.hpp"

#include <boost/core/ignore_unused.hpp>

using launchdarkly::Value;

#define AS_VALUE(x) reinterpret_cast<Value*>(x)

#define AS_OBJ_ITER(x) reinterpret_cast<ObjectIter*>(x)
#define AS_ARR_ITER(x) reinterpret_cast<ArrayIter*>(x)
#define AS_LDVALUE(x) reinterpret_cast<LDValue>(x)

struct ArrayIter {
    ArrayIter(Value* val) : val(val), iter(val->as_array().begin()) {}
    Value::Array::Iterator iter;
    Value* val;
};

struct ObjectIter {
    ObjectIter(Value* val) : val(val), iter(val->as_object().begin()) {}
    Value::Object::Iterator iter;
    Value* val;
};

LD_EXPORT(LDValue) LDValue_NewNull() {
    return AS_LDVALUE(new Value());
}

LD_EXPORT(LDValue) LDValue_NewBool(bool val) {
    return AS_LDVALUE(new Value(val));
}

LD_EXPORT(LDValue) LDValue_NewNumber(double val) {
    return AS_LDVALUE(new Value(val));
}

LD_EXPORT(LDValue) LDValue_NewString(char const* val) {
    return AS_LDVALUE(new Value(val));
}

LD_EXPORT(LDValue) LDValue_NewValue(LDValue val) {
    return AS_LDVALUE(new Value(*AS_VALUE(val)));
}

LD_EXPORT(void) LDValue_Free(LDValue val) {
    delete AS_VALUE(val);
}

LD_EXPORT(enum LDValueType) LDValue_Type(LDValue val) {
    switch (AS_VALUE(val)->type()) {
        case Value::Type::kNull:
            return LDValueType_Null;
        case Value::Type::kBool:
            return LDValueType_Bool;
        case Value::Type::kNumber:
            return LDValueType_Number;
        case Value::Type::kString:
            return LDValueType_String;
        case Value::Type::kObject:
            return LDValueType_Object;
        case Value::Type::kArray:
            return LDValueType_Array;
    }
    assert(!"Unsupported value type.");
}

LD_EXPORT(bool) LDValue_GetBool(LDValue val) {
    return AS_VALUE(val)->as_bool();
}

LD_EXPORT(double) LDValue_GetNumber(LDValue val) {
    return AS_VALUE(val)->as_double();
}

LD_EXPORT(char const*) LDValue_GetString(LDValue val) {
    return AS_VALUE(val)->as_string().c_str();
}

LD_EXPORT(unsigned int) LDValue_Count(LDValue val) {
    auto* value = AS_VALUE(val);
    switch (value->type()) {
        case Value::Type::kObject:
            return value->as_object().size();
        case Value::Type::kArray:
            return value->as_array().size();
        default:
            return 0;
    }
}

LD_EXPORT(LDValue_ArrayIter) LDValue_CreateArrayIter(LDValue val) {
    if (AS_VALUE(val)->is_array()) {
        return reinterpret_cast<LDValue_ArrayIter>(
            new ArrayIter(AS_VALUE(val)));
    }
    return nullptr;
}

LD_EXPORT(void) LDValue_ArrayIter_Next(LDValue_ArrayIter iter) {
    auto* val_iter = AS_ARR_ITER(iter);
    auto res = val_iter->iter++;
    boost::ignore_unused(res);
}

LD_EXPORT(bool) LDValue_ArrayIter_End(LDValue_ArrayIter iter) {
    auto* val_iter = AS_ARR_ITER(iter);
    return val_iter->iter == val_iter->val->as_array().end();
}

LD_EXPORT(LDValue) LdValue_ArrayIter_Value(LDValue_ArrayIter iter) {
    auto* val_iter = AS_ARR_ITER(iter);
    return AS_LDVALUE(const_cast<Value*>(&(*val_iter->iter)));
}

LD_EXPORT(void) LDValue_DestroyArrayIter(LDValue_ArrayIter iter) {
    delete AS_ARR_ITER(iter);
}

LD_EXPORT(LDValue_ObjectIter) LDValue_CreateObjectIter(LDValue val) {
    if (AS_VALUE(val)->is_object()) {
        return reinterpret_cast<LDValue_ObjectIter>(
            new ObjectIter(AS_VALUE(val)));
    }
    return nullptr;
}

LD_EXPORT(void) LDValue_ObjectIter_Next(LDValue_ObjectIter iter) {
    auto* val_iter = AS_OBJ_ITER(iter);
    auto res = val_iter->iter++;
    boost::ignore_unused(res);
}

LD_EXPORT(bool) LDValue_ObjectIter_End(LDValue_ObjectIter iter) {
    auto* val_iter = AS_OBJ_ITER(iter);
    return val_iter->iter == val_iter->val->as_object().end();
}

LD_EXPORT(LDValue) LdValue_ObjectIter_Value(LDValue_ObjectIter iter) {
    auto* val_iter = AS_OBJ_ITER(iter);
    return AS_LDVALUE(const_cast<Value*>(&val_iter->iter->second));
}

LD_EXPORT(char const*) LdValue_ObjectIter_Key(LDValue_ObjectIter iter) {
    auto* val_iter = AS_OBJ_ITER(iter);
    return val_iter->iter->first.c_str();
}

LD_EXPORT(void) LDValue_DestroyObjectIter(LDValue_ObjectIter iter) {
    delete AS_OBJ_ITER(iter);
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
