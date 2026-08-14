#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace launchdarkly::data_model {

/**
 * Identifies a specific version of data in the LaunchDarkly backend, used to
 * request incremental updates from a known point. <p> A selector is either
 * empty or contains a version number and state string that
 * were provided by a LaunchDarkly data source. Empty selectors signal that the
 * client has no existing data and requires a full payload. <p> <strong>For SDK
 * consumers implementing custom data sources:</strong> you should always use
 * std::nullopt when constructing a ChangeSet. Non-empty selectors are
 * set by LaunchDarkly's own data sources based on state received from the
 * LaunchDarkly backend, and are not meaningful when constructed externally.
 */
struct Selector {
    struct State {
        std::int64_t version;
        std::string state;
    };

    std::optional<State> value;
};

}  // namespace launchdarkly::data_model
