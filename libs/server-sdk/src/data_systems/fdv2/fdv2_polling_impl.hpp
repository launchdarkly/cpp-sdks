#pragma once

#include "../../data_components/environment_id/environment_id.hpp"
#include "../../data_interfaces/source/fdv2_source_result.hpp"

#include <launchdarkly/data_model/selector.hpp>
#include <launchdarkly/fdv2_protocol_handler.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/http_requester.hpp>
#include <launchdarkly/server_side/config/built/all_built.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace launchdarkly::server_side::data_systems {

// Build a polling HTTP GET request for the FDv2 endpoint.
network::HttpRequest MakeFDv2PollRequest(
    std::string const& polling_base_url,
    config::built::HttpProperties const& http_properties,
    data_model::Selector const& selector,
    std::optional<std::string> const& filter_key,
    Logger const& logger);

// Parse an HTTP response from the FDv2 polling endpoint through the protocol
// handler and return the appropriate result. identity is used in log messages
// to identify the caller (e.g. "FDv2 polling initializer"). If environment_id
// is present, it records the environment ID of a successful response.
data_interfaces::FDv2SourceResult HandleFDv2PollResponse(
    network::HttpResult const& res,
    FDv2ProtocolHandler* protocol_handler,
    Logger const& logger,
    std::string_view identity,
    std::shared_ptr<data_components::EnvironmentId> const& environment_id);

}  // namespace launchdarkly::server_side::data_systems
