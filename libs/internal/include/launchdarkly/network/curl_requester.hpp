#pragma once

#ifdef LD_CURL_NETWORKING

#include "http_requester.hpp"
#include "asio_requester.hpp"
#include <launchdarkly/network/curl_multi_manager.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <functional>


namespace launchdarkly::network {

using TlsOptions = config::shared::built::TlsOptions;

typedef std::function<void(const HttpResult &res)> CallbackFunction;
class CurlRequester {
public:
    CurlRequester(net::any_io_executor ctx, TlsOptions const& tls_options);

    void Request(HttpRequest request, std::function<void(const HttpResult&)> cb) const;

private:
    static void PerformRequestWithMulti(std::shared_ptr<CurlMultiManager> multi_manager,
                                       TlsOptions const& tls_options,
                                       const HttpRequest& request,
                                       std::function<void(const HttpResult&)> cb);

    net::any_io_executor ctx_;
    TlsOptions tls_options_;
    std::shared_ptr<CurlMultiManager> multi_manager_;
};

} // namespace launchdarkly::network

#endif  // LD_CURL_NETWORKING
