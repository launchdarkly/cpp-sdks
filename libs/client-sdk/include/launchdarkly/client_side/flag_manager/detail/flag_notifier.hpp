#pragma once

#include <boost/signals2.hpp>

#include "value.hpp"

namespace launchdarkly::client_side::flag_manager::detail {

class IConnection {
   public:
    virtual void disconnect() = 0;

    virtual ~IConnection() = default;
    IConnection(IConnection const& item) = delete;
    IConnection(IConnection&& item) = delete;
    IConnection& operator=(IConnection const&) = delete;
    IConnection& operator=(IConnection&&) = delete;

   protected:
    IConnection() = default;
};

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
    virtual std::unique_ptr<IConnection> flag_change(std::string const& key,
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
