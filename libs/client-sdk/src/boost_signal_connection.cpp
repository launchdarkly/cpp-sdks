#include "launchdarkly/client_side/detail/boost_signal_connection.hpp"

namespace launchdarkly::client_side::detail {

SignalConnection::SignalConnection(boost::signals2::connection connection)
    : connection_(std::move(connection)) {}

void SignalConnection::Disconnect() {
    connection_.disconnect();
}

}  // namespace launchdarkly::client_side::detail