#include "events/detail/conn_pool.hpp"
#include <iostream>
#include "network/detail/asio_requester.hpp"
namespace launchdarkly::events::detail {
ConnPool::ConnPool(boost::asio::any_io_executor io)
    : requester_(std::move(io)) {}

void ConnPool::Deliver(RequestType request) {
    requester_.Request(std::move(request), []() {});
}
}  // namespace launchdarkly::events::detail
