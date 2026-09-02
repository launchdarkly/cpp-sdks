#pragma once

#include "../data_source.hpp"
#include "../data_source_status_manager.hpp"
#include "../data_source_update_sink.hpp"
#include "ifdv2_initializer_factory.hpp"
#include "ifdv2_synchronizer_factory.hpp"

#include "../../flag_manager/flag_store.hpp"

#include <launchdarkly/async/cancellation.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/data_sources/fdv2/conditions.hpp>
#include <launchdarkly/data_sources/fdv2/source_manager.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace launchdarkly::client_side::data_sources {

// The orchestration primitives the client and server SDKs share.
using internal::data_sources::Conditions;
using internal::data_sources::FallbackConditionFactory;
using internal::data_sources::IFDv2Condition;
using internal::data_sources::IFDv2ConditionFactory;
using internal::data_sources::RecoveryConditionFactory;
using internal::data_sources::SourceSignal;

using SourceManager =
    internal::data_sources::SourceManager<IFDv2SynchronizerFactory>;

/**
 * The FDv2 data source. It runs a sequence of initializers to load flag data
 * for one evaluation context, then hands off to a synchronizer to keep that
 * data current, rotating synchronizers as they fail and recover.
 *
 * The data source is built for a single evaluation context. Changing context
 * means shutting this one down and starting another.
 *
 * Lifecycle:
 *   1. Construct.
 *   2. Call Start() exactly once. It returns immediately, and orchestration
 *      runs on the executor.
 *   3. Call ShutdownAsync() to stop. The completion runs on the executor once
 *      the source has stopped touching the store.
 *
 * Thread safety: Start, ShutdownAsync, and EnvironmentId may be called from
 * any thread.
 *
 * Orchestration:
 *
 *           Start()
 *               |
 *               v
 *     +-------------------+   no sources configured
 *     |  Anything to do?  |---------> [Done, status = kValid]
 *     +-------------------+
 *               |
 *               v
 *     +-------------------+    initializer #N returns:
 *     |  Initializer phase|      ChangeSet(no selector) -> stay, N += 1
 *     |  N = 0, 1, 2, ... |      ChangeSet(selector)    -> go to Sync
 *     |                   |      Interrupted/Terminal   -> stay, N += 1
 *     |                   |      Goodbye                -> stay, N += 1
 *     |                   |      Shutdown               -> [Closed]
 *     +-------------------+
 *               |
 *               | (N exhausted, or basis received)
 *               v
 *     +-------------------+    active synchronizer's Next returns:
 *     |  Synchronizer     |      ChangeSet      -> apply, loop
 *     |  phase            |      Interrupted    -> loop (source self-retries)
 *     |  (cyclic;         |      Goodbye        -> loop (source self-restarts)
 *     |   blocked sources |      TerminalError  -> block, advance
 *     |   are skipped)    |      Shutdown       -> [Closed]
 *     +-------------------+
 *                   ^   |     fallback condition  -> advance (with wrap)
 *                   |   |     recovery condition  -> reset to first available
 *                   +---+
 *               |
 *               | (all synchronizers blocked)
 *               v
 *     [Done; final status preserved]
 */
class FDv2DataSource final
    : public IDataSource,
      public std::enable_shared_from_this<FDv2DataSource> {
   public:
    /**
     * @param initializer_factories Build the initializers, run in order to
     *     load a basis.
     * @param synchronizer_factories Build the synchronizers, used in order to
     *     keep data current once initialization is done.
     * @param fallback_condition_factory Builds the per-synchronizer fallback
     *     condition. May be null, in which case synchronizers rotate only on
     *     terminal errors.
     * @param recovery_condition_factory Builds the per-synchronizer recovery
     *     condition. May be null, in which case the source never returns to a
     *     more-preferred synchronizer once it has fallen back.
     * @param executor Runs the orchestration.
     * @param context The evaluation context this source loads data for.
     * @param sink Receives the changesets. Non-owning. Must outlive this
     *     object.
     * @param store Supplies the selector to request incremental updates
     *     against. Non-owning. Must outlive this object.
     * @param status_manager Publishes data source status transitions.
     *     Non-owning. Must outlive this object.
     * @param logger Receives diagnostic logging.
     */
    FDv2DataSource(
        std::vector<std::unique_ptr<IFDv2InitializerFactory>>
            initializer_factories,
        std::vector<std::unique_ptr<IFDv2SynchronizerFactory>>
            synchronizer_factories,
        std::unique_ptr<IFDv2ConditionFactory> fallback_condition_factory,
        std::unique_ptr<IFDv2ConditionFactory> recovery_condition_factory,
        boost::asio::any_io_executor executor,
        Context context,
        IDataSourceUpdateSink* sink,
        flag_manager::FlagStore const* store,
        DataSourceStatusManager* status_manager,
        Logger const& logger);

    ~FDv2DataSource() override;

    void Start() override;

    void ShutdownAsync(std::function<void()> completion) override;

    /**
     * The environment the service reported the most recent payload was
     * evaluated in, or nullopt if no response has reported one.
     */
    [[nodiscard]] std::optional<std::string> EnvironmentId() const;

   private:
    /**
     * Signals the orchestration to stop and closes any active source.
     * Idempotent.
     */
    void Close();

    // Orchestration steps. Each chains the next through Future::Then, so at
    // most one step has a pending continuation at any time. mutex_ provides
    // mutual exclusion for orchestration state, and lets Close() tear down
    // active sources from any thread.

    // Publishes a status transition unless Close() has run. The client drops
    // a data source as soon as its replacement starts, and a dropped source
    // must not report over the new one.
    void PublishState(DataSourceStatus::DataSourceState state);
    void PublishState(DataSourceStatus::DataSourceState state,
                      DataSourceStatus::ErrorInfo::ErrorKind kind,
                      std::string message);

    // Applies the leading cache initializers on the calling thread, so that
    // cached flags are available as soon as Start() returns. A cache
    // initializer that does not complete synchronously is left to the chain.
    void RunCacheInitializers();

    void RunNextInitializer();
    void OnInitializerResult(FDv2SourceResult result);
    void StartSynchronizers();
    void RunSynchronizerNext();
    void OnSynchronizerResult(FDv2SourceResult result);
    void OnConditionFired(IFDv2Condition::Type type);

    // Builds the conditions to apply to the currently active synchronizer.
    // Must be called with mutex_ held. Reads source_manager_.
    std::unique_ptr<Conditions> BuildActiveConditions() const;

    // Applies a changeset to the store and records what the result reported
    // about the environment.
    void ApplyResult(FDv2SourceResult::ChangeSet change_set,
                     std::optional<std::string> environment_id,
                     bool from_cache);

    // Reports that no source can supply data, choosing the status that
    // reflects why.
    void ReportExhausted(bool any_synchronizers_configured);

    // Logger is itself thread-safe and cheap to copy.
    Logger logger_;

    // Immutable after construction.
    boost::asio::any_io_executor const executor_;
    std::vector<std::unique_ptr<IFDv2InitializerFactory>> const
        initializer_factories_;
    std::unique_ptr<IFDv2ConditionFactory> const fallback_condition_factory_;
    std::unique_ptr<IFDv2ConditionFactory> const recovery_condition_factory_;
    Context const context_;
    // True when the cache is the only source that could ever supply data, in
    // which case a cache miss still completes initialization successfully.
    bool const cache_only_;

    // Non-owning. Lifetimes guaranteed by the caller (see constructor doc).
    IDataSourceUpdateSink* const sink_;
    flag_manager::FlagStore const* const store_;
    DataSourceStatusManager* const status_manager_;

    // Set by Start() to detect repeat or concurrent calls.
    std::atomic_bool start_called_;

    // Suppresses consecutive "interrupted" logs from the active synchronizer.
    std::atomic_bool last_logged_synchronizer_interrupted_;

    // Orchestration state, protected by mutex_.
    mutable std::mutex mutex_;
    bool closed_;
    bool received_data_;
    std::optional<std::string> environment_id_;
    std::size_t initializer_index_;
    // Whether active_initializer_ reads from the local cache.
    bool active_initializer_from_cache_;
    SourceManager source_manager_;
    std::unique_ptr<IFDv2Initializer> active_initializer_;
    std::unique_ptr<IFDv2Synchronizer> active_synchronizer_;
    std::unique_ptr<Conditions> active_conditions_;
};

}  // namespace launchdarkly::client_side::data_sources
