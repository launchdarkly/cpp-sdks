#pragma once

#include "fdv2_source_result.hpp"

#include <launchdarkly/network/http_requester.hpp>

#include <optional>
#include <string>

namespace launchdarkly::client_side::data_sources {

/** The FDv2 response headers the SDK acts on. */
struct FDv2ResponseHeaders {
    /** The environment the payload was evaluated in. */
    std::optional<std::string> environment_id;
    /** Set when the service directed the SDK back to FDv1. */
    std::optional<FDv1FallbackDirective> fdv1_fallback;
};

FDv2ResponseHeaders ReadFDv2ResponseHeaders(
    network::HttpResult::HeadersType const& headers);

}  // namespace launchdarkly::client_side::data_sources
