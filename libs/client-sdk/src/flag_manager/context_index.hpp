#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <vector>

#include <boost/json.hpp>

namespace launchdarkly::client_side::flag_manager {

/**
 * Used internally to track which contexts have flag data in the persistent
 * store.
 *
 * This exists because we can't assume that the persistent store mechanism has
 * an "enumerate all the keys that exist under such-and-such prefix" capability,
 * so we need a table of contents at a fixed location. The only information
 * being tracked here is, for each flag data set that exists in storage,
 * 1. a context identifier (hashed fully-qualified key) and
 * 2. timestamp when it was last accessed, to support an LRU
 * eviction pattern.
 */
class ContextIndex {
   public:
    /**
     * Structure used to track the stored contexts.
     */
    struct IndexEntry {
        std::string id;
        std::chrono::time_point<std::chrono::system_clock> timestamp;
    };

    using Index = std::vector<IndexEntry>;
    ContextIndex() = default;
    explicit ContextIndex(Index);

    /**
     * Indicate that a context was accessed. This will add it to the index if
     * it was not already present. If it was present, then it will update the
     * timestamp for the context.
     *
     * @param id The id of the context.
     * @param timestamp The time the context is noticed.
     */
    void Notice(std::string id,
                std::chrono::time_point<std::chrono::system_clock> timestamp);

    Index const& Entries() const;

    /**
     * Prune the index returning a list of the removed context keys
     *
     * @param maxContexts The maximum number of contexts to retain.
     * @return A list of the contexts that were pruned.
     */
    std::vector<std::string> Prune(int maxContexts);

   private:
    Index index_;
};

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                ContextIndex const& index);

ContextIndex tag_invoke(boost::json::value_to_tag<ContextIndex> const& unused,
                        boost::json::value const& json_value);
}  // namespace launchdarkly::persistence
