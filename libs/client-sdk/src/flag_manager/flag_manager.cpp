#include <tl/expected.hpp>

#include "flag_manager.hpp"

namespace launchdarkly::client_side::flag_manager {

FlagManager::FlagManager(std::string const& sdk_key,
                         std::shared_ptr<IPersistence> persistence)
    : flag_updater_(flag_store_),
      persistence_updater_(sdk_key, &flag_updater_, flag_store_, persistence) {}

IDataSourceUpdateSink& FlagManager::Updater() {
    return persistence_updater_;
}

IFlagNotifier& FlagManager::Notifier() {
    return flag_updater_;
}

FlagStore const& FlagManager::Store() const {
    return flag_store_;
}

void FlagManager::LoadCache(Context const& context) {
    persistence_updater_.LoadCached(context);
}

}  // namespace launchdarkly::client_side::flag_manager
