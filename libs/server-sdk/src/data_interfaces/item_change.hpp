#pragma once

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/segment.hpp>

#include <string>
#include <variant>
#include <vector>

namespace launchdarkly::server_side::data_interfaces {

struct ItemChange {
    std::string key;
    std::variant<data_model::ItemDescriptor<data_model::Flag>,
                 data_model::ItemDescriptor<data_model::Segment>>
        object;
};

using ChangeSetData = std::vector<ItemChange>;

}  // namespace launchdarkly::server_side::data_interfaces
