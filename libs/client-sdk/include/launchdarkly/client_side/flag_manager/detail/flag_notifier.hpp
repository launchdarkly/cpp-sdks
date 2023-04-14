#pragma once

#include <boost/signals2.hpp>

#include "value.hpp"

namespace launchdarkly::client_side::flag_manager::detail {

class IFlagNotifier {
    using Connection = boost::signals2::connection;
    using ChangeHandler = void(std::shared_ptr<FlagValueChangeEvent>);
    // The FlagValueChangeEvent is in a shared pointer so that all handlers
    // can use the same instance, and the lifetime is tied to how the consumer
    // uses the event.
    using FlagChanged =
        boost::signals2::signal<void(std::shared_ptr<FlagValueChangeEvent>)>;

   public:
    /**
     * Listen for changes for the specific flag.
     * @param key The flag to listen to changes for.
     * @param signal The signal handler for the changes.
     * @return A connection which can be used to stop listening.
     */
    Connection flag_change(std::string const& key, FlagChanged signal);

    virtual ~IFlagNotifier() = default;
    IFlagNotifier(IFlagNotifier const& item) = delete;
    IFlagNotifier(IFlagNotifier&& item) = delete;
    IFlagNotifier& operator=(IFlagNotifier const&) = delete;
    IFlagNotifier& operator=(IFlagNotifier&&) = delete;

   protected:
    IFlagNotifier() = default;
};

}  // namespace launchdarkly::client_side::flag_manager::detail
