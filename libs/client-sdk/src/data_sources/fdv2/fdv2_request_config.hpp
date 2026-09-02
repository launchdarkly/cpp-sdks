#pragma once

#include <launchdarkly/config/shared/built/http_properties.hpp>

#include <string>

namespace launchdarkly::client_side::data_sources {

/**
 * How the evaluation context reaches the service on an FDv2 request.
 */
enum class FDv2ContextTransport {
    /** Base64url-encoded into the request path. The default. */
    kGetPath,
    /**
     * Serialized into the request body, keeping the context out of URL-based
     * request logs, CDN logs, and browser history.
     */
    kPostBody,
};

/**
 * The parts of an FDv2 request that do not change between calls to a source.
 * The evaluation context is one of them, since a source is built for a
 * single context and replaced when the context changes.
 */
struct FDv2RequestConfig {
    std::string base_url;
    config::shared::built::HttpProperties http_properties;
    std::string serialized_context;
    FDv2ContextTransport transport;
    bool with_reasons;
};

}  // namespace launchdarkly::client_side::data_sources
