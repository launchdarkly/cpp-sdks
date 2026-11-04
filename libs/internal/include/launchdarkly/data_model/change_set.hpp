#pragma once

#include <launchdarkly/data_model/selector.hpp>

#include <optional>
#include <string>

namespace launchdarkly::data_model {

enum class ChangeSetType {
    kFull = 0,
    kPartial = 1,
    kNone = 2,
};

template <typename T>
struct ChangeSet {
    ChangeSetType type;
    T data;
    Selector selector;

    // Environment ID reported by LaunchDarkly alongside this data, if known.
    std::optional<std::string> environment_id;
};

}  // namespace launchdarkly::data_model
