#pragma once

#include "../../data_components/change_notifier/change_notifier.hpp"
#include "../../data_components/memory_store/memory_store.hpp"
#include "../../data_components/status_notifications/data_source_status_manager.hpp"
#include "../../data_interfaces/source/ifdv2_initializer_factory.hpp"
#include "../../data_interfaces/source/ifdv2_synchronizer_factory.hpp"
#include "../../data_interfaces/system/idata_system.hpp"

#include <launchdarkly/data_model/selector.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace launchdarkly::server_side::data_systems {

/**
 * FDv2DataSystem is the IDataSystem implementation for the FDv2 protocol.
 * It runs a sequence of initializers to populate an in-memory store, then
 * hands off to a synchronizer for ongoing updates.
 *
 * Lifecycle / call order:
 *
 *   1. Construct.
 *   2. Call Initialize() exactly once. It returns immediately; orchestration
 *      runs asynchronously on the executor.
 *   3. Call IStore methods (GetFlag/GetSegment/AllFlags/AllSegments) and
 *      Initialized() / Identity() any number of times, from any thread.
 *   4. Drain the executor and join callers (see Destruction protocol).
 *   5. Destroy.
 *
 *   Initialize() may not be called more than once.
 *
 * Thread safety:
 *
 *   - GetFlag, GetSegment, AllFlags, AllSegments, Initialized, and Identity
 *     are safe to call concurrently from any thread.
 *   - Initialize() is intended to be called once from a single thread.
 *   - The destructor must run with no other method calls in flight; see
 *     Destruction protocol below.
 *
 * Destruction protocol:
 *
 *   The destructor cancels in-flight orchestration (closes the active
 *   source, emits status kOff), but does NOT block to drain executor
 *   callbacks that may already be queued. Before destroying, the caller
 *   must ensure both of:
 *
 *     1. The executor that orchestration callbacks run on has been stopped
 *        AND any thread running it has been joined. Otherwise a previously-
 *        scheduled callback may run and reference a destroyed object.
 *     2. No other thread is currently calling a method on this object.
 *
 *   The standard pattern is:
 *
 *       ioc.stop();          // no new callbacks; running work returns
 *       run_thread.join();   // wait for the executor thread to exit
 *       // Now no thread can be touching this object.
 *       // Destroy FDv2DataSystem.
 *
 *   ClientImpl's destructor uses this pattern (~ClientImpl performs
 *   ioc_.stop() and run_thread_.join() before any member is destroyed).
 *
 * Sources (initializers and synchronizers):
 *
 *   Sources are constructed lazily via the factories supplied at
 *   construction. Each factory is invoked at most once during a run of the
 *   orchestration. A source is closed and destroyed when the orchestration
 *   finishes processing a result that ends its turn, or when the
 *   FDv2DataSystem is destroyed. At any given moment at most one source is
 *   active.
 *
 * Selector tracking:
 *
 *   FDv2DataSystem tracks the most-recent non-empty selector seen on a
 *   ChangeSet and passes it into each subsequent synchronizer Next() call.
 *   The initial selector is empty.
 *
 * Orchestration phases:
 *
 *           Initialize()
 *               |
 *               v
 *     +-------------------+   no factories given
 *     |  Offline?         |---------> [Done, status = kValid]
 *     +-------------------+
 *               |
 *               v
 *     +-------------------+    initializer #N returns:
 *     |  Initializer phase|      ChangeSet(no selector) -> stay, N += 1
 *     |  N = 0, 1, 2, ... |      ChangeSet(selector)    -> go to Sync
 *     |                   |      Interrupted/Terminal   -> stay, N += 1
 *     |                   |      Goodbye/Timeout        -> stay, N += 1
 *     |                   |      Shutdown               -> [Closed]
 *     +-------------------+
 *               |
 *               | (N exhausted, or basis received)
 *               v
 *     +-------------------+    synchronizer #M's Next returns:
 *     |  Synchronizer     |      ChangeSet      -> apply, loop
 *     |  phase            |      Interrupted    -> loop (source self-retries)
 *     |  M = 0, 1, 2, ... |      Timeout        -> loop
 *     |                   |      Goodbye/Term   -> M += 1
 *     |                   |      Shutdown       -> [Closed]
 *     +-------------------+
 *               |
 *               | (M exhausted)
 *               v
 *     [Done; final status preserved]
 *
 *     Calling the destructor at any time -> [Closed; status kOff].
 *
 * Status transitions:
 *
 *   kInitializing (initial) -> kValid on first successful ChangeSet apply.
 *                              kInterrupted on Interrupted / TerminalError
 *                              (filtered to kInitializing while still in
 *                              the initializer phase if not yet Valid).
 *   kValid                  -> kInterrupted on errors; kOff in destructor.
 *   kInterrupted            -> kValid on next successful ChangeSet apply;
 *                              kOff in destructor.
 *   kOff                    -> terminal.
 */
class FDv2DataSystem final : public data_interfaces::IDataSystem {
   public:
    /**
     * Constructs the data system.
     *
     * @param initializer_factories Factories that build initializers, run in
     *     order during the initialization phase.
     * @param synchronizer_factories Factories that build synchronizers, used
     *     in order for ongoing updates after initialization.
     * @param ioc Executor on which orchestration callbacks run.
     * @param status_manager Non-owning. Must outlive this object; the caller
     *     is responsible for ensuring this. Used to publish data-source
     *     status transitions.
     * @param logger Used for diagnostic logging. Held by value (Logger is
     *     internally thread-safe and cheap to copy).
     */
    FDv2DataSystem(
        std::vector<std::unique_ptr<data_interfaces::IFDv2InitializerFactory>>
            initializer_factories,
        std::vector<std::unique_ptr<data_interfaces::IFDv2SynchronizerFactory>>
            synchronizer_factories,
        boost::asio::any_io_executor ioc,
        data_components::DataSourceStatusManager* status_manager,
        Logger const& logger);

    ~FDv2DataSystem() override;

    FDv2DataSystem(FDv2DataSystem const&) = delete;
    FDv2DataSystem(FDv2DataSystem&&) = delete;
    FDv2DataSystem& operator=(FDv2DataSystem const&) = delete;
    FDv2DataSystem& operator=(FDv2DataSystem&&) = delete;

    /**
     * Returns the flag descriptor for the given key, or nullptr if no flag
     * with that key is currently in the store.
     */
    std::shared_ptr<data_model::FlagDescriptor> GetFlag(
        std::string const& key) const override;

    /**
     * Returns the segment descriptor for the given key, or nullptr if no
     * segment with that key is currently in the store.
     */
    std::shared_ptr<data_model::SegmentDescriptor> GetSegment(
        std::string const& key) const override;

    /**
     * Returns all flag descriptors currently in the store, keyed by flag key.
     * The returned map is a snapshot; subsequent updates are not reflected.
     */
    std::unordered_map<std::string, std::shared_ptr<data_model::FlagDescriptor>>
    AllFlags() const override;

    /**
     * Returns all segment descriptors currently in the store, keyed by
     * segment key. The returned map is a snapshot; subsequent updates are
     * not reflected.
     */
    std::unordered_map<std::string,
                       std::shared_ptr<data_model::SegmentDescriptor>>
    AllSegments() const override;

    /**
     * Returns a display-suitable name for the data system, used in
     * diagnostic logging.
     */
    std::string const& Identity() const override;

    /**
     * Starts the orchestration: runs initializers in order on the executor,
     * then hands off to a synchronizer for ongoing updates. Returns
     * immediately; orchestration runs asynchronously. Must be called exactly
     * once, before any IStore method is invoked.
     */
    void Initialize() override;

    /**
     * Returns true once the in-memory store has been populated for the first
     * time. Naming follows the IDataSystem base interface (predates the
     * IsX() convention).
     */
    bool Initialized() const override;

   private:
    /**
     * Signals the orchestration loop to stop and closes any active source.
     * Idempotent. Called from the destructor.
     */
    void Close();

    // Orchestration-loop methods. Each step chains the next via Future::Then,
    // so at most one is in flight at a time. mutex_ guards shared state
    // against the destructor's Close() running concurrently with a callback.

    void RunNextInitializer();
    void OnInitializerResult(data_interfaces::FDv2SourceResult result);
    void StartSynchronizers();
    void RunSynchronizerNext();
    void OnSynchronizerResult(data_interfaces::FDv2SourceResult result);

    // Applies a typed FDv2 changeset to the in-memory store and updates the
    // tracked selector if the changeset's selector is non-empty.
    void ApplyChangeSet(
        data_model::ChangeSet<data_interfaces::ChangeSetData> change_set);

    // Logger is itself thread-safe and cheap to copy.
    Logger logger_;

    // Immutable after construction.
    boost::asio::any_io_executor const ioc_;
    std::vector<std::unique_ptr<data_interfaces::IFDv2InitializerFactory>> const
        initializer_factories_;
    std::vector<
        std::unique_ptr<data_interfaces::IFDv2SynchronizerFactory>> const
        synchronizer_factories_;
    // Non-owning. Lifetime guaranteed by the caller (see constructor doc).
    data_components::DataSourceStatusManager* const status_manager_;

    // Internally synchronized.
    data_components::MemoryStore store_;
    // Holds references to store_; declared after it so destruction order is
    // safe.
    data_components::ChangeNotifier change_notifier_;

    // Orchestration state, guarded by mutex_.
    std::mutex mutex_;
    bool closed_;
    data_model::Selector selector_;
    std::size_t initializer_index_;
    std::size_t synchronizer_index_;
    std::unique_ptr<data_interfaces::IFDv2Initializer> active_initializer_;
    std::unique_ptr<data_interfaces::IFDv2Synchronizer> active_synchronizer_;
};

}  // namespace launchdarkly::server_side::data_systems
