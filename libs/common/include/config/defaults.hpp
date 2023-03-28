#pragma

#include "config/service_endpoints.hpp"

namespace launchdarkly::config {

struct ClientSDK;
struct ServerSDK;

template <typename SDK>
struct Defaults {};

template <>
struct Defaults<ClientSDK> {
    ServiceEndpoints endpoints() const;
};

template <>
struct Defaults<ServerSDK> {
    ServiceEndpoints endpoints() const;
};

}  // namespace launchdarkly::config
