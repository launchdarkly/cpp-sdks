#pragma once

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/segment.hpp>
#include <launchdarkly/data_model/selector.hpp>

#include <string>
#include <variant>
#include <vector>

namespace launchdarkly::data_model {

struct FDv2Change {
    std::string key;
    std::variant<ItemDescriptor<Flag>, ItemDescriptor<Segment>> object;
};

struct FDv2ChangeSet {
    enum class Type {
        kFull = 0,
        kPartial = 1,
        kNone = 2,
    };

    Type type;
    std::vector<FDv2Change> changes;
    Selector selector;
};

}  // namespace launchdarkly::data_model
