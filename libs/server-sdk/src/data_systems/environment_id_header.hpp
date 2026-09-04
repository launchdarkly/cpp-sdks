#pragma once

#include <optional>
#include <string>
#include <utility>

#include <boost/beast/http/fields.hpp>

#include <launchdarkly/network/http_requester.hpp>

namespace launchdarkly::server_side::data_systems {

inline constexpr char const* kEnvironmentIdHeader = "X-LD-EnvID";

inline std::optional<std::string> ReadEnvironmentId(
    network::HttpResult::HeadersType const& headers) {
    auto const it = headers.find(kEnvironmentIdHeader);
    if (it == headers.end() || it->second.empty()) {
        return std::nullopt;
    }
    return it->second;
}

inline std::optional<std::string> ReadEnvironmentId(
    boost::beast::http::fields const& headers) {
    auto const it = headers.find(kEnvironmentIdHeader);
    if (it == headers.end() || it->value().empty()) {
        return std::nullopt;
    }
    return std::string(it->value().data(), it->value().size());
}

}  // namespace launchdarkly::server_side::data_systems
