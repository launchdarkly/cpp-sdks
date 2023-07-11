#pragma once

#include <launchdarkly/connection.hpp>

#include <functional>
#include <memory>
#include <set>
#include <string>

namespace launchdarkly::server_side {

/**
 * Interface to allow listening for flag changes. Notification events should
 * be distributed after the store has been updated.
 */
class IChangeNotifier {
   public:
    using ChangeSet = std::set<std::string>;
    using ChangeHandler = std::function<void(std::shared_ptr<ChangeSet>)>;

    /**
     * Listen for changes to flag configuration. The change handler will be
     * called with a set of affected flag keys. Changes include flags whose
     * dependencies (either other flags, or segments) changed.
     *
     * @param signal The handler for the changes.
     * @return A connection which can be used to stop listening.
     */
    virtual std::unique_ptr<IConnection> OnFlagChange(
        ChangeHandler handler) = 0;

    virtual ~IChangeNotifier() = default;
    IChangeNotifier(IChangeNotifier const& item) = delete;
    IChangeNotifier(IChangeNotifier&& item) = delete;
    IChangeNotifier& operator=(IChangeNotifier const&) = delete;
    IChangeNotifier& operator=(IChangeNotifier&&) = delete;

   protected:
    IChangeNotifier() = default;
};

}  // namespace launchdarkly::server_side
