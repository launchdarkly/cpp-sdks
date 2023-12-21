#pragma once

#include <launchdarkly/server_side/integrations/data_reader/iserialized_data_reader.hpp>

#include <chrono>
#include <memory>

namespace launchdarkly::server_side::config::built {

struct LazyLoadConfig {
    /**
     * \brief Specifies the action taken when a data item's TTL expires.
     *
     * The values must not be changed to ensure backwards compatibility
     * with the C API.
     */
    enum class EvictionPolicy {
        /* No action taken; eviction is disabled. Stale items will be used
         * in evaluations if they cannot be refreshed. */
        Disabled = 0
    };

    EvictionPolicy eviction_policy;
    std::chrono::milliseconds refresh_ttl;
    std::shared_ptr<integrations::ISerializedDataReader> source;
};
}  // namespace launchdarkly::server_side::config::built
