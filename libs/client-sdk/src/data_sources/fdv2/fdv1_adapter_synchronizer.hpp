#pragma once

#include "../data_source.hpp"
#include "../data_source_status_manager.hpp"
#include "../data_source_update_sink.hpp"
#include "ifdv2_synchronizer.hpp"

#include <launchdarkly/async/promise.hpp>
#include <launchdarkly/connection.hpp>
#include <launchdarkly/context.hpp>

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace launchdarkly::client_side::data_sources {

/**
 * Presents an FDv1 data source as an FDv2 synchronizer, so that the
 * orchestrator can run it while the service has directed the SDK away from
 * FDv2.
 *
 * FDv1 has no selectors, so the changesets this reports carry none. The
 * orchestrator therefore never asks the service for a delta against data
 * FDv1 supplied.
 *
 * Thread safety: Next() and Close() may be called from any thread. Only one
 * Next() may be outstanding at a time.
 */
class FDv1AdapterSynchronizer final : public IFDv2Synchronizer {
   public:
    /**
     * Builds the wrapped FDv1 source. Called once during construction with
     * the sink and status manager the source must report through, both of
     * which the adapter keeps alive for the source's lifetime.
     */
    using SourceBuilder =
        std::function<std::shared_ptr<IDataSource>(IDataSourceUpdateSink*,
                                                   DataSourceStatusManager*)>;

    explicit FDv1AdapterSynchronizer(SourceBuilder source_builder);

    ~FDv1AdapterSynchronizer() override;

    async::Future<FDv2SourceResult> Next(
        data_model::Selector selector) override;

    void Close() override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    /**
     * Holds the result queue and the pending Next() promise. Shared with the
     * wrapped source's sink and status subscription. Thread-safe.
     */
    class State {
       public:
        explicit State(async::Future<std::monostate> closed);

        async::Future<FDv2SourceResult> GetNext();

        /**
         * Resolves any pending Next() with Shutdown and clears it, so that a
         * caller abandoned by Close() is not left waiting.
         */
        void ResolvePendingAsShutdown();

        void Notify(FDv2SourceResult result);

       private:
        // Finished once the owning adapter's Close() has run. Read in Notify
        // to drop late results.
        async::Future<std::monostate> const closed_;

        mutable std::mutex mutex_;
        // Both protected by mutex_.
        std::optional<async::Promise<FDv2SourceResult>> pending_promise_;
        std::deque<FDv2SourceResult> result_queue_;
    };

    /**
     * Turns the FDv1 source's Init and Upsert calls into FDv2 changesets
     * queued on State. Thread-safe (delegates to State).
     */
    class ConvertingSink final : public IDataSourceUpdateSink {
       public:
        explicit ConvertingSink(std::weak_ptr<State> state);

        void Init(
            Context const& context,
            std::unordered_map<std::string, ItemDescriptor> data) override;
        void Upsert(Context const& context,
                    std::string key,
                    ItemDescriptor item) override;
        void Apply(Context const& context,
                   FlagChangeSet change_set,
                   bool from_cache) override;

       private:
        std::weak_ptr<State> state_;
    };

    // Thread-safe primitive. Declared before state_ so state_'s constructor
    // can take a future from it.
    async::Promise<std::monostate> close_promise_;

    // shared_ptr so async callbacks that fire after this is destroyed can
    // hold their own reference.
    std::shared_ptr<State> const state_;
    std::shared_ptr<ConvertingSink> const sink_;
    std::shared_ptr<DataSourceStatusManager> const status_manager_;
    std::unique_ptr<IConnection> const status_subscription_;

    std::shared_ptr<IDataSource> const fdv1_source_;

    // Serializes Start and ShutdownAsync on fdv1_source_ across concurrent
    // Next() and Close() calls.
    std::mutex lifecycle_mutex_;
    // Protected by lifecycle_mutex_. Set when Next() starts the source, or
    // when Close() runs first, so that a later Next() cannot start it.
    bool started_ = false;
};

}  // namespace launchdarkly::client_side::data_sources
