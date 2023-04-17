#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "launchdarkly/client_side/data_source_update_sink.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_change_event.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_manager.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_notifier.hpp"

namespace launchdarkly::client_side::flag_manager::detail {

class Connection : public IConnection {
   public:
    Connection(boost::signals2::connection connection);
    void disconnect() override;

   private:
    boost::signals2::connection connection_;
};

class FlagUpdater : public IDataSourceUpdateSink, public IFlagNotifier {
   public:
    FlagUpdater(FlagManager& flag_manager);
    void init(std::unordered_map<std::string, ItemDescriptor> data) override;
    void upsert(std::string key, ItemDescriptor item) override;

    /**
     * Listen for changes for the specific flag.
     * @param key The flag to listen to.
     * @param handler The handler to signal when the flag changes.
     * @return A Connection which can be used to stop listening for changes
     * to the flag using this handler.
     */
    std::unique_ptr<IConnection> flag_change(
        std::string const& key,
        std::function<void(std::shared_ptr<FlagValueChangeEvent>)> const&
            handler) override;

   private:
    bool has_listeners() const;

    FlagManager& flag_manager_;
    std::unordered_map<
        std::string,
        boost::signals2::signal<void(std::shared_ptr<FlagValueChangeEvent>)>>
        signals_;

    // Recursive mutex so that has_listeners can non-conditionally lock
    // the mutex. Otherwise a pre-condition for the call would be holding
    // the mutex, which is more difficult to keep consistent over the code
    // lifetime.
    mutable std::recursive_mutex signal_mutex_;
    void dispatch_event(FlagValueChangeEvent event);
};

}  // namespace launchdarkly::client_side::flag_manager::detail
