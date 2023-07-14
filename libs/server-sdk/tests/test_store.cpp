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

std::unique_ptr<data_store::IDataStore> TestData() {
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

    store->Upsert("flagWithMatchesOpOnGroups", Flag(R"({
        "key": "flagWithMatchesOpOnGroups",
        "version": 42,
        "on": true,
        "targets": [],
        "rules": [
            {
                "variation": 0,
                "id": "6a7755ac-e47a-40ea-9579-a09dd5f061bd",
                "clauses": [
                    {
                        "attribute": "groups",
                        "op": "matches",
                        "values": [
                            "^\\w+"
                        ],
                        "negate": false
                    }
                ],
                "trackEvents": true
            }
        ],
        "prerequisites": [],
        "fallthrough": {"variation": 1},
        "offVariation": 0,
        "variations": [false, true],
        "clientSide": true,
        "clientSideAvailability": {
            "usingEnvironmentId": true,
            "usingMobileKey": true
        },
        "salt": "salty",
        "trackEvents": false,
        "trackEventsFallthrough": true,
        "debugEventsUntilDate": 1500000000
    })"));

    store->Upsert("flagWithMatchesOpOnKinds", Flag(R"({
            "key": "flagWithMatchesOpOnKinds",
            "version": 42,
            "on": true,
            "targets": [],
            "rules": [
                {
                    "variation": 0,
                    "id": "6a7755ac-e47a-40ea-9579-a09dd5f061bd",
                    "clauses": [
                        {
                            "attribute": "kind",
                            "op": "matches",
                            "values": [
                                "^[ou]"
                            ],
                            "negate": false
                        }
                    ],
                    "trackEvents": true
                }
            ],
            "prerequisites": [],
            "fallthrough": {"variation": 1},
            "offVariation": 0,
            "variations": [false, true],
            "clientSide": true,
            "clientSideAvailability": {
                "usingEnvironmentId": true,
                "usingMobileKey": true
            },
            "salt": "salty",
            "trackEvents": false,
            "trackEventsFallthrough": true,
            "debugEventsUntilDate": 1500000000
        })"));
    return store;
}

}  // namespace launchdarkly::server_side::test_store
