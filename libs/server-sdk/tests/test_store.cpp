#include "test_store.hpp"

#include "data_store/memory_store.hpp"

#include <launchdarkly/serialization/json_flag.hpp>

namespace launchdarkly::server_side::test_store {

std::unique_ptr<data_store::IDataStore> Empty() {
    auto store = std::make_unique<data_store::MemoryStore>();
    store->Init({});
    return store;
}

data_store::FlagDescriptor Flag(char const* json) {
    auto val = boost::json::value_to<
        tl::expected<std::optional<data_model::Flag>, JsonError>>(
        boost::json::parse(json));
    assert(val.has_value());
    assert(val.value().has_value());
    return data_store::FlagDescriptor{val.value().value()};
}

std::unique_ptr<data_store::IDataStore> Make() {
    auto store = std::make_unique<data_store::MemoryStore>();
    store->Init({});
    store->Upsert("flagWithTarget", Flag(R"(
    {
        "key": "flagWithTarget",
        "version": 42,
        "on": false,
        "targets": [{
            "values": ["bob"],
            "variation": 0
        }],
        "rules": [],
        "prerequisites": [],
        "fallthrough": {"variation": 1},
        "offVariation": 0,
        "variations": [false, true],
        "clientSide": true,
        "clientSideAvailability": {
            "usingEnvironmentId": true,
            "usingMobileKey": true
        },
        "salt": "salty"
    })"));

    return store;
}

}  // namespace launchdarkly::server_side::test_store
