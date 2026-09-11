#include "fdv2_response_headers.hpp"

#include <boost/algorithm/string/predicate.hpp>

namespace launchdarkly::client_side::data_sources {

static char const* const kEnvironmentIdHeader = "X-LD-EnvId";
static char const* const kFDv1FallbackHeader = "X-LD-FD-Fallback";
static char const* const kFDv1FallbackTtlHeader = "X-LD-FD-Fallback-TTL";

FDv2ResponseHeaders ReadFDv2ResponseHeaders(
    network::HttpResult::HeadersType const& headers) {
    FDv2ResponseHeaders result;

    if (auto const it = headers.find(kEnvironmentIdHeader);
        it != headers.end()) {
        result.environment_id = it->second;
    }

    auto const fallback = headers.find(kFDv1FallbackHeader);
    if (fallback == headers.end() ||
        !boost::iequals(fallback->second, "true")) {
        return result;
    }

    auto const ttl = headers.find(kFDv1FallbackTtlHeader);
    result.fdv1_fallback =
        ttl == headers.end()
            ? FDv1FallbackDirective::DefaultTtl()
            : FDv1FallbackDirective::FromServiceTtl(ttl->second);

    return result;
}

FDv2ResponseHeaders ReadFDv2ResponseHeaders(
    boost::beast::http::response_header<> const& headers) {
    FDv2ResponseHeaders result;

    if (auto const it = headers.find(kEnvironmentIdHeader);
        it != headers.end()) {
        result.environment_id = std::string{it->value()};
    }

    auto const fallback = headers.find(kFDv1FallbackHeader);
    if (fallback == headers.end() ||
        !boost::iequals(fallback->value(), "true")) {
        return result;
    }

    auto const ttl = headers.find(kFDv1FallbackTtlHeader);
    result.fdv1_fallback =
        ttl == headers.end()
            ? FDv1FallbackDirective::DefaultTtl()
            : FDv1FallbackDirective::FromServiceTtl(
                  std::string_view{ttl->value().data(), ttl->value().size()});

    return result;
}

}  // namespace launchdarkly::client_side::data_sources
