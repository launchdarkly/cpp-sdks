#pragma once

#include "../../data_interfaces/destination/idestination.hpp"
#include "../../data_interfaces/source/idata_synchronizer.hpp"
#include "../../data_interfaces/source/ifdv2_synchronizer.hpp"

#include <launchdarkly/async/promise.hpp>

#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <variant>

namespace launchdarkly::server_side::data_systems {

/**
 * Adapts an FDv1 IDataSynchronizer to the IFDv2Synchronizer interface.
 *
 * FDv1 Init/Upsert callbacks delivered through an internal IDestination are
 * translated into FDv2SourceResult::ChangeSet results, with empty selectors
 * and fdv1_fallback = false (the directive does not re-fire from FDv1 data).
 *
 * Threading: Next() and Close() may be called from any thread; only one
 * Next() may be outstanding at a time. The adapter blocks in its destructor
 * waiting for the FDv1 source's ShutdownAsync completion, so no callbacks
 * are in flight when the wrapped source is destroyed.
 */
class FDv1AdapterSynchronizer final
    : public data_interfaces::IFDv2Synchronizer {
   public:
    explicit FDv1AdapterSynchronizer(
        std::unique_ptr<data_interfaces::IDataSynchronizer> fdv1_source);

    ~FDv1AdapterSynchronizer() override;

    async::Future<data_interfaces::FDv2SourceResult> Next(
        data_model::Selector selector) override;
    void Close() override;
    [[nodiscard]] std::string const& Identity() const override;

   private:
    /**
     * Holds the lifecycle, result queue, and pending Next() promise; shared
     * with the FDv1 source's IDestination via the inner ConvertingDestination.
     * All methods are thread-safe.
     */
    class State {
       public:
        // Returns true if this call transitioned Initial → Started; false if
        // already started or already closed. Used to gate the one-time
        // StartAsync call on the wrapped FDv1 source.
        bool TryStart();

        // Marks the state closed and returns whether the source was started
        // before the transition (so the caller knows whether ShutdownAsync
        // needs to be called).
        bool MarkClosed();

        async::Future<data_interfaces::FDv2SourceResult> GetNext();

        // Resolves any pending Next() promise with Shutdown and clears it.
        // Called on the close path so the abandoned promise doesn't leave
        // potential continuations dangling.
        void ResolvePendingAsShutdown();

        void Notify(data_interfaces::FDv2SourceResult result);

       private:
        // Protected by mutex_.
        mutable std::mutex mutex_;
        bool started_ = false;
        bool closed_ = false;
        std::optional<async::Promise<data_interfaces::FDv2SourceResult>>
            pending_promise_;
        std::deque<data_interfaces::FDv2SourceResult> result_queue_;
    };

    /**
     * Translates FDv1 IDestination callbacks into FDv2 results queued on
     * State. Thread-safe (delegates to State).
     */
    class ConvertingDestination final : public data_interfaces::IDestination {
       public:
        explicit ConvertingDestination(std::weak_ptr<State> state);
        void Init(data_model::SDKDataSet data_set) override;
        void Upsert(std::string const& key,
                    data_model::FlagDescriptor flag) override;
        void Upsert(std::string const& key,
                    data_model::SegmentDescriptor segment) override;
        [[nodiscard]] std::string const& Identity() const override;

       private:
        std::weak_ptr<State> state_;
    };

    // const after construction.
    std::shared_ptr<State> const state_;
    std::unique_ptr<ConvertingDestination> const destination_;
    std::unique_ptr<data_interfaces::IDataSynchronizer> const fdv1_source_;

    // Thread-safe primitive.
    async::Promise<std::monostate> close_promise_;
};

}  // namespace launchdarkly::server_side::data_systems
