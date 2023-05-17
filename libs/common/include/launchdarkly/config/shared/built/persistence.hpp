#pragma once

#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/persistence/persistence.hpp>

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
struct Persistence<ServerSDK> {};

}  // namespace launchdarkly::config::shared::built