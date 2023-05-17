// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/bindings/c/value.h>
#include <launchdarkly/bindings/c/iter.hpp>
#include <launchdarkly/value.hpp>
#include "../../../src/c_binding_helpers.hpp"

using launchdarkly::Value;

#define AS_VALUE(x) reinterpret_cast<Value*>(x)

#define AS_OBJ_ITER(x) \
    reinterpret_cast<IteratorBinding<Value::Object::Iterator>*>(x)
#define AS_ARR_ITER(x) \
    reinterpret_cast<IteratorBinding<Value::Array::Iterator>*>(x)
#define AS_LDVALUE(x) reinterpret_cast<LDValue>(x)

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
    switch (AS_VALUE(val)->Type()) {
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
    LD_ASSERT(!"Unsupported value type.");
}

LD_EXPORT(bool) LDValue_GetBool(LDValue val) {
    return AS_VALUE(val)->AsBool();
}

LD_EXPORT(double) LDValue_GetNumber(LDValue val) {
    return AS_VALUE(val)->AsDouble();
}

LD_EXPORT(char const*) LDValue_GetString(LDValue val) {
    return AS_VALUE(val)->AsString().c_str();
}

LD_EXPORT(unsigned int) LDValue_Count(LDValue val) {
    auto* value = AS_VALUE(val);
    switch (value->Type()) {
        case Value::Type::kObject:
            return value->AsObject().Size();
        case Value::Type::kArray:
            return value->AsArray().Size();
        default:
            return 0;
    }
}

LD_EXPORT(LDValue_ArrayIter) LDValue_CreateArrayIter(LDValue val) {
    if (AS_VALUE(val)->IsArray()) {
        auto& array = AS_VALUE(val)->AsArray();
        return reinterpret_cast<LDValue_ArrayIter>(
            new IteratorBinding<Value::Array::Iterator>{array.begin(),
                                                        array.end()});
    }
    return nullptr;
}

LD_EXPORT(void) LDValue_ArrayIter_Next(LDValue_ArrayIter iter) {
    AS_ARR_ITER(iter)->Next();
}

LD_EXPORT(bool) LDValue_ArrayIter_End(LDValue_ArrayIter iter) {
    auto* val_iter = AS_ARR_ITER(iter);
    return val_iter->End();
}

LD_EXPORT(LDValue) LdValue_ArrayIter_Value(LDValue_ArrayIter iter) {
    auto* val_iter = AS_ARR_ITER(iter);
    return AS_LDVALUE(const_cast<Value*>(&(*val_iter->iter)));
}

LD_EXPORT(void) LDValue_DestroyArrayIter(LDValue_ArrayIter iter) {
    delete AS_ARR_ITER(iter);
}

LD_EXPORT(LDValue_ObjectIter) LDValue_CreateObjectIter(LDValue val) {
    if (AS_VALUE(val)->IsObject()) {
        auto& obj = AS_VALUE(val)->AsObject();
        return reinterpret_cast<LDValue_ObjectIter>(
            new IteratorBinding<Value::Object::Iterator>{obj.begin(),
                                                         obj.end()});
    }
    return nullptr;
}

LD_EXPORT(void) LDValue_ObjectIter_Next(LDValue_ObjectIter iter) {
    AS_OBJ_ITER(iter)->Next();
}

LD_EXPORT(bool) LDValue_ObjectIter_End(LDValue_ObjectIter iter) {
    auto* val_iter = AS_OBJ_ITER(iter);
    return val_iter->End();
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
