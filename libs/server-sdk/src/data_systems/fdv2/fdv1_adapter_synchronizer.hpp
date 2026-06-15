#pragma once

#include "../../data_components/status_notifications/data_source_status_manager.hpp"
#include "../../data_interfaces/destination/idestination.hpp"
#include "../../data_interfaces/source/idata_synchronizer.hpp"
#include "../../data_interfaces/source/ifdv2_synchronizer.hpp"

#include <launchdarkly/async/promise.hpp>
#include <launchdarkly/connection.hpp>

#include <deque>
#include <functional>
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
 * Next() may be outstanding at a time. Member declaration order ensures
 * the wrapped FDv1 source destructs before destination_ and state_, so any
 * in-flight FDv1 callbacks land on live objects during teardown. This
 * relies on the wrapped IDataSynchronizer blocking on its in-flight work
 * in its destructor.
 */
class FDv1AdapterSynchronizer final
    : public data_interfaces::IFDv2Synchronizer {
   public:
    using SourceBuilder =
        std::function<std::shared_ptr<data_interfaces::IDataSynchronizer>(
            data_components::DataSourceStatusManager&)>;

    /**
     * @param source_builder Called once during construction with the
     *                       adapter's status manager. Returns the wrapped
     *                       FDv1 source, which must be constructed against
     *                       the provided manager as its status sink.
     */
    explicit FDv1AdapterSynchronizer(SourceBuilder source_builder);

    ~FDv1AdapterSynchronizer() override;

    async::Future<data_interfaces::FDv2SourceResult> Next(
        data_model::Selector selector) override;
    void Close() override;
    [[nodiscard]] std::string const& Identity() const override;

   private:
    /**
     * Holds the result queue and pending Next() promise; shared with the
     * FDv1 source's IDestination via the inner ConvertingDestination.
     * All methods are thread-safe.
     */
    class State {
       public:
        explicit State(async::Future<std::monostate> closed_future);

        async::Future<data_interfaces::FDv2SourceResult> GetNext();

        // Resolves any pending Next() promise with Shutdown and clears it.
        // Called on the close path so the abandoned promise doesn't leave
        // potential continuations dangling.
        void ResolvePendingAsShutdown();

        void Notify(data_interfaces::FDv2SourceResult result);

       private:
        // Finished once the owning FDv1AdapterSynchronizer's close_promise_
        // is resolved. Read in Notify to drop late results.
        async::Future<std::monostate> const closed_future_;

        mutable std::mutex mutex_;
        // Protected by mutex_.
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

    // Thread-safe primitive. Declared before state_ so state_'s constructor
    // can take a future from it.
    async::Promise<std::monostate> close_promise_;

    // shared_ptr so async callbacks that may fire after this is destroyed
    // can hold their own reference.
    std::shared_ptr<State> const state_;
    std::unique_ptr<ConvertingDestination> const destination_;

    std::unique_ptr<data_components::DataSourceStatusManager> const
        status_manager_;
    std::unique_ptr<IConnection> const status_subscription_;

    std::shared_ptr<data_interfaces::IDataSynchronizer> const fdv1_source_;

    // Serializes StartAsync and ShutdownAsync on fdv1_source_ across
    // concurrent Next() and Close() calls.
    std::mutex lifecycle_mutex_;
    // Protected by lifecycle_mutex_. Set when Next() calls StartAsync, or
    // when Close() runs without a prior start (to gate any later Next()
    // from calling StartAsync after Close).
    bool started_ = false;
};

}  // namespace launchdarkly::server_side::data_systems
