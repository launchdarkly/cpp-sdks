#pragma once

#include "../../data_interfaces/source/fdv2_source_result.hpp"

#include <launchdarkly/data_model/selector.hpp>
#include <launchdarkly/fdv2_protocol_handler.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/http_requester.hpp>
#include <launchdarkly/server_side/config/built/all_built.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace launchdarkly::server_side::data_systems {

// Build a polling HTTP GET request for the FDv2 endpoint.
network::HttpRequest MakeFDv2PollRequest(
    config::built::ServiceEndpoints const& endpoints,
    config::built::HttpProperties const& http_properties,
    data_model::Selector const& selector,
    std::optional<std::string> const& filter_key);

// Parse an HTTP response from the FDv2 polling endpoint through the protocol
// handler and return the appropriate result. identity is used in log messages
// to identify the caller (e.g. "FDv2 polling initializer").
data_interfaces::FDv2SourceResult HandleFDv2PollResponse(
    network::HttpResult const& res,
    FDv2ProtocolHandler& protocol_handler,
    Logger const& logger,
    std::string_view identity);

}  // namespace launchdarkly::server_side::data_systems
