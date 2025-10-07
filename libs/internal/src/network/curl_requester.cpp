#include "../../include/launchdarkly/network/curl_requester.hpp"
#include "launchdarkly/network/curl_requester.hpp"

namespace launchdarkly::network {
    CurlRequester::CurlRequester(net::any_io_executor ctx, TlsOptions const& tls_options) {}

    void CurlRequester::Request(HttpRequest request, std::function<void(const HttpResult&)> cb) {}
}