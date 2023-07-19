#pragma once

#include <chrono>
#include <cstddef>
#include <memory>

#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/persistence/persistence.hpp>
#include <launchdarkly/persistence/persistent_store_core.hpp>

namespace launchdarkly::config::shared::built {

template <typename SDK>
struct Persistence;

template <>
struct Persistence<ClientSDK> {
    bool disable_persistence;
    std::shared_ptr<IPersistence> implementation;
    std::size_t max_contexts_;
};

template <>
struct Persistence<ServerSDK> {
    std::shared_ptr<persistence::IPersistentStoreCore> implementation;
    std::chrono::seconds cache_refresh_time;
    bool active_eviction;
    std::chrono::seconds eviction_interval;
};

}  // namespace launchdarkly::config::shared::built
