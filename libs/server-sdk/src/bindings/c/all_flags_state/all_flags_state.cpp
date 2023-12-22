#include <launchdarkly/server_side/bindings/c/all_flags_state/all_flags_state.h>
#include <launchdarkly/server_side/serialization/json_all_flags_state.hpp>

#include <launchdarkly/detail/c_binding_helpers.hpp>

#include <boost/json/serialize.hpp>
#include <boost/json/value_from.hpp>

#include <string.h>

#define TO_ALLFLAGS(ptr) (reinterpret_cast<AllFlagsState*>(ptr))
#define FROM_ALLFLAGS(ptr) (reinterpret_cast<LDAllFlagsState>(ptr))

#define TO_VALUE(ptr) (reinterpret_cast<Value*>(ptr))
#define FROM_VALUE(ptr) (reinterpret_cast<LDValue>(ptr))

using namespace launchdarkly;
using namespace launchdarkly::server_side;

LD_EXPORT(void) LDAllFlagsState_Free(LDAllFlagsState state) {
    delete TO_ALLFLAGS(state);
}

LD_EXPORT(bool) LDAllFlagsState_Valid(LDAllFlagsState state) {
    LD_ASSERT_NOT_NULL(state);

    return TO_ALLFLAGS(state)->Valid();
}

LD_EXPORT(char*)
LDAllFlagsState_SerializeJSON(LDAllFlagsState state) {
    LD_ASSERT_NOT_NULL(state);

    auto json_value = boost::json::value_from(*TO_ALLFLAGS(state));
    std::string json_str = boost::json::serialize(json_value);

    return strdup(json_str.c_str());
}

LD_EXPORT(LDValue)
LDAllFlagsState_Value(LDAllFlagsState state, char const* flag_key) {
    LD_ASSERT_NOT_NULL(state);
    LD_ASSERT_NOT_NULL(flag_key);

    auto const& values = TO_ALLFLAGS(state)->Values();

    std::unordered_map<std::string, Value>::const_iterator iter =
        values.find(flag_key);
    if (iter == values.end()) {
        return FROM_VALUE(const_cast<Value*>(&Value::Null()));
    }

    Value const& val_ref = iter->second;

    return FROM_VALUE(const_cast<Value*>(&val_ref));
}

LD_EXPORT(LDValue)
LDAllFlagsState_Map(LDAllFlagsState state) {
    LD_ASSERT_NOT_NULL(state);

    auto const& values = TO_ALLFLAGS(state)->Values();

    std::map<std::string, Value> map;
    for (auto const& pair : values) {
        map.emplace(pair.first, pair.second);
    }

    return FROM_VALUE(new Value(std::move(map)));
}
