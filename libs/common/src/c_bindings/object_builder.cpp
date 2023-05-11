// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include "launchdarkly/c_bindings/object_builder.h"
#include "launchdarkly/value.hpp"

#include <map>
#include <string>

using launchdarkly::Value;

#define AS_MAP(x) reinterpret_cast<std::map<std::string, Value>*>(x)
#define AS_VALUE(x) reinterpret_cast<Value*>(x)
#define AS_LDVALUE(x) reinterpret_cast<LDValue>(x)

LD_EXPORT(LDObjectBuilder) LDObjectBuilder_New() {
    return reinterpret_cast<LDObjectBuilder>(
        new std::map<std::string, Value>());
}

LD_EXPORT(void) LDObjectBuilder_Free(LDObjectBuilder builder) {
    delete AS_MAP(builder);
}

LD_EXPORT(void)
LDObjectBuilder_Add(LDObjectBuilder builder, char const* key, LDValue val) {
    auto map = AS_MAP(builder);
    map->emplace(key, std::move(*AS_VALUE(val)));
    LDValue_Free(val);
}

LD_EXPORT(LDValue) LDObjectBuilder_Build(LDObjectBuilder builder) {
    auto map = AS_MAP(builder);
    auto value = new Value(std::move(*map));
    delete map;
    return AS_LDVALUE(value);
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
