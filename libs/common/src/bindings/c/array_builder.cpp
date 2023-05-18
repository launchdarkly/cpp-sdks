// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/bindings/c/array_builder.h>
#include <launchdarkly/detail/c_binding_helpers.hpp>
#include <launchdarkly/value.hpp>

#include <vector>

using launchdarkly::Value;

#define AS_VECTOR(x) reinterpret_cast<std::vector<Value>*>(x)
#define AS_VALUE(x) reinterpret_cast<Value*>(x)
#define AS_LDVALUE(x) reinterpret_cast<LDValue>(x)

LD_EXPORT(LDArrayBuilder) LDArrayBuilder_New() {
    return reinterpret_cast<LDArrayBuilder>(new std::vector<Value>());
}

LD_EXPORT(void) LDArrayBuilder_Free(LDArrayBuilder array_builder) {
    delete AS_VECTOR(array_builder);
}

LD_EXPORT(void) LDArrayBuilder_Add(LDArrayBuilder array_builder, LDValue val) {
    LD_ASSERT_NOT_NULL(array_builder);
    LD_ASSERT_NOT_NULL(val);

    auto vector = AS_VECTOR(array_builder);
    vector->emplace_back(std::move(*AS_VALUE(val)));
    LDValue_Free(val);
}

LD_EXPORT(LDValue) LDArrayBuilder_Build(LDArrayBuilder array_builder) {
    LD_ASSERT_NOT_NULL(array_builder);

    auto vector = AS_VECTOR(array_builder);
    auto value = new Value(std::move(*vector));
    delete vector;
    return AS_LDVALUE(value);
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
