#pragma once

#include "../../data_interfaces/source/ifdv2_synchronizer.hpp"
#include "../../data_interfaces/source/ifdv2_synchronizer_factory.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace launchdarkly::server_side::data_systems {

/**
 * Manages a list of synchronizer factories together with per-factory state
 * (Available / Blocked) and the index of the currently active factory.
 *
 * Iteration is cyclic: NextSynchronizer advances past the end and wraps to
 * the beginning, skipping any factory in the Blocked state. A factory enters
 * the Blocked state when BlockCurrentSynchronizer is called (typically on a
 * terminal error). Once blocked, a factory is not eligible to be built again
 * until unblocked.
 *
 * ResetSourceIndex causes the next call to start iteration at index 0 — used
 * by recovery, which wants to fall back to the most-preferred Available
 * synchronizer.
 *
 * Factories whose IsFDv1Fallback() returns true start in the Blocked state.
 *
 * Not thread-safe. The caller is responsible for serializing all calls.
 */
class SourceManager {
   public:
    explicit SourceManager(
        std::vector<std::unique_ptr<data_interfaces::IFDv2SynchronizerFactory>>
            factories);

    /**
     * Advances to the next Available synchronizer factory (wrapping past the
     * end), builds a synchronizer instance from it, and records that factory
     * as the current one for subsequent queries. Returns nullptr if no
     * Available factory exists.
     */
    std::unique_ptr<data_interfaces::IFDv2Synchronizer> NextSynchronizer();

    /**
     * Marks the currently tracked factory as Blocked. No-op if no factory is
     * currently tracked.
     */
    void BlockCurrentSynchronizer();

    /**
     * Resets the iteration cursor so that the next call to NextSynchronizer
     * begins searching from index 0.
     */
    void ResetSourceIndex();

    /**
     * Blocks every non-FDv1 factory and unblocks the FDv1 fallback factory,
     * if one was configured. Resets the iteration cursor so the next call to
     * NextSynchronizer returns the FDv1 fallback. If no FDv1 fallback factory
     * was configured, every factory is left blocked.
     */
    void SwitchToFDv1Fallback();

    /**
     * Returns true if the currently tracked factory is the first Available
     * factory in the list. Returns false if no factory is currently tracked.
     */
    [[nodiscard]] bool IsPrimeSynchronizer() const;

    /**
     * Returns the count of factories not in the Blocked state.
     */
    [[nodiscard]] std::size_t AvailableSynchronizerCount() const;

    /**
     * Returns the total number of factories configured at construction
     * (including any currently in the Blocked state). Constant for the
     * lifetime of the SourceManager.
     */
    [[nodiscard]] std::size_t SynchronizerCount() const;

    /**
     * Returns true if the currently tracked factory is the FDv1 fallback
     * synchronizer.
     */
    [[nodiscard]] bool IsCurrentSynchronizerFDv1Fallback() const;

    SourceManager(SourceManager const&) = delete;
    SourceManager(SourceManager&&) = delete;
    SourceManager& operator=(SourceManager const&) = delete;
    SourceManager& operator=(SourceManager&&) = delete;
    ~SourceManager() = default;

   private:
    enum class State { kAvailable, kBlocked };

    struct SynchronizerFactoryWithState {
        std::unique_ptr<data_interfaces::IFDv2SynchronizerFactory> factory;
        State state = State::kAvailable;
        bool is_fdv1_fallback = false;
    };

    std::vector<SynchronizerFactoryWithState> synchronizers_;
    // Iteration cursor; -1 means "start at 0 on next call".
    int synchronizer_index_ = -1;
    // Index of the most recently returned factory; -1 if none.
    int current_factory_index_ = -1;
};

}  // namespace launchdarkly::server_side::data_systems
