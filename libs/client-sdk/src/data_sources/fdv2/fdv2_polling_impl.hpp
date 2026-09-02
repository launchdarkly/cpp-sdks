#pragma once

#include "fdv2_request_config.hpp"
#include "fdv2_source_result.hpp"

#include <launchdarkly/data_model/selector.hpp>
#include <launchdarkly/fdv2_protocol_handler.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/http_requester.hpp>

#include <string_view>

namespace launchdarkly::client_side::data_sources {

/**
 * Builds a request to the FDv2 client polling endpoint. A non-empty selector
 * is sent as the basis for a delta response.
 *
 * No conditional-request validator is sent. A validator cached for one
 * context must never be replayed for another, and the basis parameter already
 * gives the service what it needs to answer with a delta.
 */
network::HttpRequest MakeFDv2PollRequest(FDv2RequestConfig const& config,
                                         data_model::Selector const& selector);

/**
 * Interprets a response from the FDv2 polling endpoint, feeding its events
 * through the protocol handler.
 *
 * @param protocol_handler Accumulates the response's events. A poll is a
 * complete transfer cycle, so callers pass a handler used for this response
 * alone.
 * @param logger Receives a description of any failure.
 * @param identity Names the caller in log messages.
 */
FDv2SourceResult HandleFDv2PollResponse(network::HttpResult const& res,
                                        FDv2ProtocolHandler* protocol_handler,
                                        Logger const& logger,
                                        std::string_view identity);

}  // namespace launchdarkly::client_side::data_sources
