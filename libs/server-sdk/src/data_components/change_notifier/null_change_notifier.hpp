#pragma once

#include <launchdarkly/server_side/change_notifier.hpp>
#include <launchdarkly/signals/boost_signal_connection.hpp>

#include <memory>

namespace launchdarkly::server_side::data_components {

/**
 * A change notifier for data systems that never observe flag changes. Handlers
 * are accepted but never called.
 *
 * Thread-safe: holds no state.
 */
class NullChangeNotifier final : public IChangeNotifier {
   public:
    NullChangeNotifier() = default;

    std::unique_ptr<IConnection> OnFlagChange(ChangeHandler handler) override {
        return std::make_unique<
            launchdarkly::internal::signals::SignalConnection>(
            boost::signals2::connection());
    }
};

}  // namespace launchdarkly::server_side::data_components
