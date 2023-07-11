#include <launchdarkly/signals/boost_signal_connection.hpp>

namespace launchdarkly::internal::signals {

SignalConnection::SignalConnection(boost::signals2::connection connection)
    : connection_(std::move(connection)) {}

void SignalConnection::Disconnect() {
    connection_.disconnect();
}

}  // namespace launchdarkly::internal::signals
