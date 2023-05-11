#pragma once

#include <launchdarkly/client_side/connection.hpp>
#include <launchdarkly/client_side/flag_manager/detail/flag_change_event.hpp>
#include <launchdarkly/value.hpp>

namespace launchdarkly::client_side::flag_manager::detail {

/**
 * Interface to allow listening for flag changes. Notification events should
 * be distributed after the store has been updated. Meaning that the "new"
 * value will correspond to the current value in the store, and the "old" value
 * will be what the value was before the update.
 */
class IFlagNotifier {
   public:
    // The FlagValueChangeEvent is in a shared pointer so that all handlers
    // can use the same instance, and the lifetime is tied to how the consumer
    // uses the event.
    using ChangeHandler =
        std::function<void(std::shared_ptr<FlagValueChangeEvent>)>;

    /**
     * Listen for changes for the specific flag.
     * @param key The flag to listen to changes for.
     * @param signal The handler for the changes.
     * @return A connection which can be used to stop listening.
     */
    virtual std::unique_ptr<IConnection> OnFlagChange(
        std::string const& key,
        ChangeHandler handler) = 0;

    virtual ~IFlagNotifier() = default;
    IFlagNotifier(IFlagNotifier const& item) = delete;
    IFlagNotifier(IFlagNotifier&& item) = delete;
    IFlagNotifier& operator=(IFlagNotifier const&) = delete;
    IFlagNotifier& operator=(IFlagNotifier&&) = delete;

   protected:
    IFlagNotifier() = default;
};

}  // namespace launchdarkly::client_side::flag_manager::detail
