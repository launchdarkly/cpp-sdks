#include "events/detail/conn_pool.hpp"
#include <iostream>
namespace launchdarkly::events::detail {
ConnPool::ConnPool() {}

void ConnPool::async_write(RequestType request) {
    std::cout << "making an HTTP request with body" << request.body().data()
              << "\n";
}
}  // namespace launchdarkly::events::detail
