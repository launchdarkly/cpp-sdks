#pragma once

#include <boost/asio/any_io_executor.hpp>
#include "network/detail/http_requester.hpp"
#include "network/detail/asio_requester.hpp"

namespace launchdarkly::events::detail {

class ConnPool {
   public:
    using RequestType = network::detail::HttpRequest;
    ConnPool(boost::asio::any_io_executor io);
    void Deliver(RequestType request);
   private:
    network::detail::AsioRequester requester_;

};
}  // namespace launchdarkly::events::detail
