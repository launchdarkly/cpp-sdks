#pragma once

#include <launchdarkly/server_side/integrations/big_segments/ibig_segment_store.hpp>

#include <chrono>
#include <cstddef>
#include <memory>

namespace launchdarkly::server_side::config::built {

struct BigSegmentsConfig {
    std::shared_ptr<integrations::IBigSegmentStore> store;
    std::size_t context_cache_size;
    std::chrono::milliseconds context_cache_time;
    std::chrono::milliseconds status_poll_interval;
    std::chrono::milliseconds stale_after;
};

}  // namespace launchdarkly::server_side::config::built
