#pragma once

#include "flag_persistence.hpp"
#include "flag_store.hpp"
#include "flag_updater.hpp"

#include <launchdarkly/context.hpp>

namespace launchdarkly::client_side::flag_manager {

/**
 * Owns the flag store and the update pipeline that feeds it.
 *
 * Thread-safe: each part it exposes is itself thread-safe.
 */
class FlagManager {
   public:
    FlagManager(std::string const& sdk_key,
                Logger& logger,
                std::size_t max_cached_contexts,
                std::shared_ptr<IPersistence> persistence);
    IDataSourceUpdateSink& Updater();
    IFlagNotifier& Notifier();
    FlagStore const& Store() const;

    /** Returns the local cache the SDK persists flag data to. */
    FlagPersistence& Cache();

    void LoadCache(Context const& context);

    /**
     * Forgets the selector for the data currently held, leaving the data in
     * place. Called when the evaluation context changes, since a selector
     * describes one context's data and is never reused for another.
     */
    void ClearSelector();

   private:
    FlagStore flag_store_;
    FlagUpdater flag_updater_;
    FlagPersistence persistence_updater_;
};

}  // namespace launchdarkly::client_side::flag_manager
