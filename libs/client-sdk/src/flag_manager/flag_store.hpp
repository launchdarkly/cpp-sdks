#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../data_sources/data_source_update_sink.hpp"
#include "context_index.hpp"

#include <launchdarkly/client_side/flag_change_event.hpp>
#include <launchdarkly/data_model/selector.hpp>
#include <launchdarkly/persistence/persistence.hpp>

namespace launchdarkly::client_side::flag_manager {

/**
 * Holds the flag data the SDK evaluates against, plus the selector
 * identifying the state of that data.
 *
 * Thread-safe: every method may be called from any thread, and each call is
 * atomic, so a reader never observes a partially applied write.
 */
class FlagStore {
   public:
    void Init(std::unordered_map<std::string, ItemDescriptor> const& data);
    void Upsert(std::string const& key, ItemDescriptor item);

    /**
     * Applies a changeset as a single unit.
     *
     * The changeset's selector becomes the store's selector. A full or
     * partial changeset carrying no selector clears it instead, because the
     * resulting data no longer corresponds to a state the service can
     * compute deltas against. A "none" changeset leaves the selector alone.
     *
     * The first full data set the store receives is reported as no changes at
     * all, since it is what the SDK starts from rather than a change to it.
     *
     * @param compute_changes Whether to report the resulting value changes.
     * Pass false when nothing is listening for them, to skip the comparison.
     * @return The value changes the apply produced, in an unspecified order.
     * Empty when compute_changes is false.
     */
    std::vector<FlagValueChangeEvent> Apply(FlagChangeSet const& change_set,
                                            bool compute_changes);

    /**
     * The selector for the data currently held, or an empty selector if that
     * data did not come with one. An empty selector means the SDK has no
     * verified basis on which to request incremental updates.
     */
    [[nodiscard]] data_model::Selector CurrentSelector() const;

    /**
     * Forgets the current selector, leaving the flag data in place. Called
     * when the evaluation context changes, since a selector describes one
     * context's data and is never reused for another.
     */
    void ClearSelector();

    /**
     * Attempts to get a flag by key from the current flags.
     *
     * @param flag_key The flag to get.
     * @return A shared_ptr to the value if present. A null shared_ptr if the
     * item is not present.
     */
    std::shared_ptr<ItemDescriptor> Get(std::string const& flag_key) const;

    /**
     * Gets all the current flags.
     *
     * @return All of the current flags.
     */
    std::unordered_map<std::string, std::shared_ptr<ItemDescriptor>> GetAll()
        const;

   private:
    void UpdateData(
        std::unordered_map<std::string, ItemDescriptor> const& data);

    // Both protected by data_mutex_.
    std::unordered_map<std::string, std::shared_ptr<ItemDescriptor>> data_;
    data_model::Selector selector_;

    mutable std::mutex data_mutex_;
};

/**
 * Computes the value-change event for one flag moving from previous to
 * current. Either pointer may be null when the flag is absent. Returns
 * nullopt when the evaluated value did not change.
 */
std::optional<FlagValueChangeEvent> ComputeFlagChange(
    std::string const& key,
    ItemDescriptor const* previous,
    ItemDescriptor const* current);

}  // namespace launchdarkly::client_side::flag_manager
