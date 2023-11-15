#pragma once

#include <launchdarkly/config/shared/sdks.hpp>

namespace launchdarkly::config::shared::built {

struct LazyLoadConfig {
    /**
     * \brief Specifies the action taken when a data item's TTL expires.
     */
    enum class EvictionPolicy {
        /* No action taken; eviction is disabled. Stale items will be used
         * in evaluations if they cannot be refreshed. */
        Disabled = 0
    };

    EvictionPolicy eviction_policy;
    std::chrono::milliseconds eviction_ttl;
    DataSourceConfig<ServerSDK> source;
};
}  // namespace launchdarkly::config::shared::built
