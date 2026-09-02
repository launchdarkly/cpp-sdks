#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace launchdarkly::internal::data_sources {

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
 *
 * @tparam Factory The SDK's synchronizer factory interface, which must supply
 * IsFDv1Fallback() and a Build() returning a smart pointer to a synchronizer.
 */
template <typename Factory>
class SourceManager {
   public:
    using SynchronizerPtr = decltype(std::declval<Factory&>().Build());

    explicit SourceManager(std::vector<std::unique_ptr<Factory>> factories) {
        synchronizers_.reserve(factories.size());
        for (auto& factory : factories) {
            bool const is_fdv1_fallback = factory->IsFDv1Fallback();
            synchronizers_.push_back(SynchronizerFactoryWithState{
                std::move(factory),
                is_fdv1_fallback ? State::kBlocked : State::kAvailable,
                is_fdv1_fallback});
        }
    }

    /**
     * Advances to the next Available synchronizer factory (wrapping past the
     * end), builds a synchronizer instance from it, and records that factory
     * as the current one for subsequent queries. Returns nullptr if no
     * Available factory exists.
     */
    SynchronizerPtr NextSynchronizer() {
        if (synchronizers_.empty()) {
            current_factory_index_ = -1;
            return nullptr;
        }
        for (std::size_t visited = 0; visited < synchronizers_.size();
             ++visited) {
            synchronizer_index_ = (synchronizer_index_ + 1) %
                                  static_cast<int>(synchronizers_.size());
            if (synchronizers_[synchronizer_index_].state ==
                State::kAvailable) {
                current_factory_index_ = synchronizer_index_;
                return synchronizers_[synchronizer_index_].factory->Build();
            }
        }
        current_factory_index_ = -1;
        return nullptr;
    }

    /**
     * Marks the currently tracked factory as Blocked. No-op if no factory is
     * currently tracked.
     */
    void BlockCurrentSynchronizer() {
        if (current_factory_index_ >= 0) {
            synchronizers_[current_factory_index_].state = State::kBlocked;
        }
    }

    /**
     * Resets the iteration cursor so that the next call to NextSynchronizer
     * begins searching from index 0.
     */
    void ResetSourceIndex() { synchronizer_index_ = -1; }

    /**
     * Blocks every non-FDv1 factory and unblocks the FDv1 fallback factory,
     * if one was configured. Resets the iteration cursor so the next call to
     * NextSynchronizer returns the FDv1 fallback. If no FDv1 fallback factory
     * was configured, every factory is left blocked.
     */
    void SwitchToFDv1Fallback() {
        for (auto& entry : synchronizers_) {
            entry.state =
                entry.is_fdv1_fallback ? State::kAvailable : State::kBlocked;
        }
        synchronizer_index_ = -1;
    }

    /**
     * Returns synchronizer state to the initial configuration, including
     * unblocking factories previously blocked by terminal errors.
     */
    void SwitchBackToFDv2() {
        for (auto& entry : synchronizers_) {
            entry.state =
                entry.is_fdv1_fallback ? State::kBlocked : State::kAvailable;
        }
        synchronizer_index_ = -1;
    }

    /**
     * Returns true if the currently tracked factory is the first Available
     * factory in the list. Returns false if no factory is currently tracked.
     */
    [[nodiscard]] bool IsPrimeSynchronizer() const {
        for (std::size_t i = 0; i < synchronizers_.size(); ++i) {
            if (synchronizers_[i].state == State::kAvailable) {
                return synchronizer_index_ == static_cast<int>(i);
            }
        }
        return false;
    }

    /**
     * Returns the count of factories not in the Blocked state.
     */
    [[nodiscard]] std::size_t AvailableSynchronizerCount() const {
        std::size_t count = 0;
        for (auto const& s : synchronizers_) {
            if (s.state == State::kAvailable) {
                ++count;
            }
        }
        return count;
    }

    /**
     * Returns the total number of factories configured at construction
     * (including any currently in the Blocked state). Constant for the
     * lifetime of the SourceManager.
     */
    [[nodiscard]] std::size_t SynchronizerCount() const {
        return synchronizers_.size();
    }

    /**
     * Returns true if the currently tracked factory is the FDv1 fallback
     * synchronizer.
     */
    [[nodiscard]] bool IsCurrentSynchronizerFDv1Fallback() const {
        return current_factory_index_ >= 0 &&
               synchronizers_[current_factory_index_].is_fdv1_fallback;
    }

    SourceManager(SourceManager const&) = delete;
    SourceManager(SourceManager&&) = delete;
    SourceManager& operator=(SourceManager const&) = delete;
    SourceManager& operator=(SourceManager&&) = delete;
    ~SourceManager() = default;

   private:
    enum class State { kAvailable, kBlocked };

    struct SynchronizerFactoryWithState {
        std::unique_ptr<Factory> factory;
        State state = State::kAvailable;
        bool is_fdv1_fallback = false;
    };

    std::vector<SynchronizerFactoryWithState> synchronizers_;
    // Iteration cursor; -1 means "start at 0 on next call".
    int synchronizer_index_ = -1;
    // Index of the most recently returned factory; -1 if none.
    int current_factory_index_ = -1;
};

}  // namespace launchdarkly::internal::data_sources
