#include "test_store.hpp"

#include "data_store/memory_store.hpp"

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

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

data_store::SegmentDescriptor Segment(char const* json) {
    auto val = boost::json::value_to<
        tl::expected<std::optional<data_model::Segment>, JsonError>>(
        boost::json::parse(json));
    assert(val.has_value());
    assert(val.value().has_value());
    return data_store::SegmentDescriptor{val.value().value()};
}

std::unique_ptr<data_store::IDataStore> TestData() {
    auto store = std::make_unique<data_store::MemoryStore>();
    store->Init({});

    store->Upsert("segmentWithNoRules", Segment(R"({
        "key": "segmentWithNoRules",
        "included": ["alice"],
        "excluded": [],
        "rules": [],
        "salt": "salty",
        "version": 1
        })"));
    store->Upsert("segmentWithRuleMatchesUserAlice", Segment(R"({
        "key": "segmentWithRuleMatchesUserAlice",
        "included": [],
        "excluded": [],
       "rules": [{
             "id": "rule-1",
             "clauses": [{
                 "attribute": "key",
                 "negate": false,
                 "op": "in",
                 "values": ["alice"],
                 "contextKind": "user"
             }]
        }],
        "salt": "salty",
        "version": 1
        })"));

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

    store->Upsert("cycleFlagA", Flag(R"({
               "key": "cycleFlagA",
               "targets": [],
               "rules": [],
               "salt": "salty",
               "prerequisites": [{
                   "key": "cycleFlagB",
                   "variation": 0
               }],
               "on": true,
               "fallthrough": {"variation": 0},
               "offVariation": 1,
               "variations": [true, false]
        })"));
    store->Upsert("cycleFlagB", Flag(R"({
                "key": "cycleFlagB",
                "targets": [],
                "rules": [],
                "salt": "salty",
                "prerequisites": [{
                    "key": "cycleFlagA",
                    "variation": 0
                }],
                "on": true,
                "fallthrough": {"variation": 0},
                "offVariation": 1,
                "variations": [true, false]
        })"));

    store->Upsert("flagWithExperiment", Flag(R"({
                "key": "flagWithExperiment",
               "version": 42,
               "on": true,
               "targets": [],
               "rules": [],
               "prerequisites": [],
               "fallthrough": {
                 "rollout": {
                   "kind": "experiment",
                   "seed": 61,
                   "variations": [
                     {"variation": 0, "weight": 10000, "untracked": false},
                     {"variation": 1, "weight": 20000, "untracked": false},
                     {"variation": 0, "weight": 70000, "untracked": true}
                   ]
                 }
               },
               "offVariation": 0,
               "variations": [false, true],
               "clientSide": true,
               "clientSideAvailability": {
                   "usingEnvironmentId": true,
                   "usingMobileKey": true
               },
               "salt": "salty",
               "trackEvents": false,
               "trackEventsFallthrough": false,
               "debugEventsUntilDate": 1500000000
        })"));
    store->Upsert("flagWithExperimentTargetingContext", Flag(R"({
                "key": "flagWithExperimentTargetingContext",
                "version": 42,
                "on": true,
                "targets": [],
                "rules": [],
                "prerequisites": [],
                "fallthrough": {
                  "rollout": {
                    "kind": "experiment",
                    "contextKind": "org",
                    "seed": 61,
                    "variations": [
                      {"variation": 0, "weight": 10000, "untracked": false},
                      {"variation": 1, "weight": 20000, "untracked": false},
                      {"variation": 0, "weight": 70000, "untracked": true}
                    ]
                  }
                },
                "offVariation": 0,
                "variations": [false, true],
                "clientSide": true,
                "clientSideAvailability": {
                    "usingEnvironmentId": true,
                    "usingMobileKey": true
                },
                "salt": "salty",
                "trackEvents": false,
                "trackEventsFallthrough": false,
                "debugEventsUntilDate": 1500000000
        })"));
    store->Upsert("flagWithSegmentMatchRule", Flag(R"({
                "key": "flagWithSegmentMatchRule",
                "version": 42,
                "on": true,
                "targets": [],
                "rules": [{
                   "id": "match-rule",
                   "clauses": [{
                       "contextKind": "user",
                       "attribute": "key",
                       "negate": false,
                       "op": "segmentMatch",
                       "values": ["segmentWithNoRules"]
                   }],
                   "variation": 0,
                   "trackEvents": false
                }],
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

    store->Upsert("flagWithSegmentMatchesUserAlice", Flag(R"({
                "key": "flagWithSegmentMatchesUserAlice",
                "version": 42,
                "on": true,
                "targets": [],
                "rules": [{
                   "id": "match-rule",
                   "clauses": [{
                       "op": "segmentMatch",
                       "values": ["segmentWithRuleMatchesUserAlice"]
                   }],
                   "variation": 0,
                   "trackEvents": false
                }],
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
