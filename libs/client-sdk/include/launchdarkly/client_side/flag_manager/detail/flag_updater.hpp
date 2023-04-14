#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "flag_manager.hpp"
#include "launchdarkly/client_side/data_source_update_sink.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_change_event.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_notifier.hpp"

namespace launchdarkly::client_side::flag_manager::detail {

class FlagUpdater : public IDataSourceUpdateSink, public IFlagNotifier {
   public:
    using Connection = boost::signals2::connection;

    FlagUpdater(FlagManager& flag_manager);
    void init(std::unordered_map<std::string, ItemDescriptor> data) override;
    void upsert(std::string key, ItemDescriptor) override;

    /**
     * Listen for changes for the specific flag.
     * @tparam F The type of handler.
     * @param key The flag to listen to.
     * @param handler The handler to signal when the flag changes.
     * @return A Connection which can be used to stop listening for changes
     * to the flag using this handler.
     */
    template <typename F>
    Connection flag_change(std::string const& key, F&& handler) {
        std::lock_guard{signal_mutex_};
        return signals_[key].connect(std::forward(handler));
    }

   private:
    bool has_listeners() const;

    FlagManager& flag_manager_;
    std::unordered_map<
        std::string,
        boost::signals2::signal<void(std::shared_ptr<FlagValueChangeEvent>)>>
        signals_;

    mutable std::recursive_mutex signal_mutex_;
    void dispatch_event(FlagValueChangeEvent event);
};

}  // namespace launchdarkly::client_side::flag_manager::detail
