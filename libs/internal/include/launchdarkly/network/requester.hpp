#pragma once

#include "http_requester.hpp"
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <functional>
#include <memory>
#include <boost/asio/any_io_executor.hpp>

namespace launchdarkly::network {

namespace net = boost::asio;
using TlsOptions = config::shared::built::TlsOptions;

// Forward declaration to hide implementation details
class IRequesterImpl;

/**
 * Requester provides HTTP request functionality using either CURL or Boost.Beast
 * depending on the LD_CURL_NETWORKING compile-time flag.
 *
 * When LD_CURL_NETWORKING is ON: Uses CurlRequester (CURL-based implementation)
 * When LD_CURL_NETWORKING is OFF: Uses AsioRequester (Boost.Beast-based implementation)
 *
 * The implementation choice is made at library compile-time and hidden from users
 * via the pimpl idiom to avoid ABI issues.
 */
class Requester {
public:
    Requester(net::any_io_executor ctx, TlsOptions const& tls_options);
    ~Requester();

    // Move-only type
    Requester(Requester&&) noexcept;
    Requester& operator=(Requester&&) noexcept;
    Requester(const Requester&) = delete;
    Requester& operator=(const Requester&) = delete;

    void Request(HttpRequest request, std::function<void(const HttpResult&)> cb);

private:
    std::unique_ptr<IRequesterImpl> impl_;
};

} // namespace launchdarkly::network
