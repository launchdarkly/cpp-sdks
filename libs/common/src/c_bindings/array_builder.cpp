#include "c_bindings/array_builder.h"
#include "value.hpp"

#include <vector>

using launchdarkly::Value;

#define AS_VECTOR(x) reinterpret_cast<std::vector<Value>*>(x)
#define AS_VALUE(x) reinterpret_cast<Value*>(x)

LD_EXPORT(LDArrayBuilder) LDArrayBuilder_New() {
    return new std::vector<Value>();
}
LD_EXPORT(void) LDArrayBuilder_Free(LDArrayBuilder array_builder) {
    delete AS_VECTOR(array_builder);
}

LD_EXPORT(void) LDArrayBuilder_Add(LDArrayBuilder array_builder, LDValue val) {
    auto vector = AS_VECTOR(array_builder);
    vector->emplace_back(std::move(*AS_VALUE(val)));
    LDValue_Free(val);
}

LD_EXPORT(LDValue) LDArrayBuilder_Build(LDArrayBuilder array_builder) {
    auto vector = AS_VECTOR(array_builder);
    auto value = new Value(std::move(*vector));
    delete vector;
    return value;
}
