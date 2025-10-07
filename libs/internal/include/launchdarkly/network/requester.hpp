#pragma once

#include "http_requester.hpp"
#include "asio_requester.hpp"
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <functional>


namespace launchdarkly::network {

using TlsOptions = config::shared::built::TlsOptions;

typedef std::function<void(const HttpResult &res)> CallbackFunction;
class Requester {
    AsioRequester innerRequester_;
public:
    Requester(net::any_io_executor ctx, TlsOptions const& tls_options): innerRequester_(ctx, tls_options) {}

    void Request(HttpRequest request, std::function<void(const HttpResult&)> cb) {
        innerRequester_.Request(request, [cb](const HttpResult &res) {
            cb(res);
        });
    }
};

} // namespace launchdarkly::network
