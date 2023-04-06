#include "events/detail/conn_pool.hpp"
#include <iostream>
namespace launchdarkly::events::detail {
ConnPool::ConnPool() {}

void ConnPool::async_write(RequestType request) {
    std::cout << "making an HTTP request to " << request.target() << std::endl;
}
}  // namespace launchdarkly::events::detail
