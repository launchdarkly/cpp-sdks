#pragma once

#include <boost/beast/http.hpp>

namespace launchdarkly::events::detail {

class ConnPool {
   public:
    using RequestType =
        boost::beast::http::request<boost::beast::http::string_body>;
    ConnPool();
    void async_write(RequestType request);
};
}  // namespace launchdarkly::events::detail
