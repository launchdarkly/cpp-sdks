#pragma once

#include <launchdarkly/data_model/selector.hpp>

namespace launchdarkly::data_model {

enum class ChangeSetType {
    kFull = 0,
    kPartial = 1,
    kNone = 2,
};

template <typename T>
struct ChangeSet {
    ChangeSetType type;
    Selector selector;
    T data;
};

}  // namespace launchdarkly::data_model
