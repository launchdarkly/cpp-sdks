#pragma once

#include <launchdarkly/data_model/change_set.hpp>
#include <launchdarkly/data_model/selector.hpp>

#include <boost/json/value.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace launchdarkly::data_model {

struct FDv2Change {
    enum class ChangeType { kPut, kDelete };

    ChangeType change_type;
    std::string kind;
    std::string key;
    uint64_t version;
    boost::json::value object;  // set for kPut; unused for kDelete
};

struct FDv2ChangeSet {
    ChangeSetType type;
    std::vector<FDv2Change> changes;
    Selector selector;
};

}  // namespace launchdarkly::data_model
