#pragma once

#include "flag_persistence.hpp"
#include "flag_store.hpp"
#include "flag_updater.hpp"

#include <launchdarkly/context.hpp>

namespace launchdarkly::client_side::flag_manager {

class FlagManager {
   public:
    FlagManager(std::string const& sdk_key,
                Logger& logger,
                std::shared_ptr<IPersistence> persistence);
    IDataSourceUpdateSink& Updater();
    IFlagNotifier& Notifier();
    FlagStore const& Store() const;

    void LoadCache(Context const& context);

   private:
    FlagStore flag_store_;
    FlagUpdater flag_updater_;
    FlagPersistence persistence_updater_;
};

}  // namespace launchdarkly::client_side::flag_manager