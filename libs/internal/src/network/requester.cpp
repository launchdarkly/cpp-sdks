#include <launchdarkly/network/requester.hpp>

#ifdef LD_CURL_NETWORKING
#include <launchdarkly/network/curl_requester.hpp>
#else
#include <launchdarkly/network/asio_requester.hpp>
#endif

namespace launchdarkly::network {

// Abstract interface for the implementation
class IRequesterImpl {
public:
    virtual ~IRequesterImpl() = default;
    virtual void Request(HttpRequest request, std::function<void(const HttpResult&)> cb) = 0;
};

#ifdef LD_CURL_NETWORKING
// CURL-based implementation
class CurlRequesterImpl : public IRequesterImpl {
public:
    CurlRequesterImpl(net::any_io_executor ctx, TlsOptions const& tls_options)
        : requester_(ctx, tls_options) {}

    void Request(HttpRequest request, std::function<void(const HttpResult&)> cb) override {
        requester_.Request(std::move(request), std::move(cb));
    }

private:
    CurlRequester requester_;
};
#else
// Boost.Beast-based implementation
class AsioRequesterImpl : public IRequesterImpl {
public:
    AsioRequesterImpl(net::any_io_executor ctx, TlsOptions const& tls_options)
        : requester_(ctx, tls_options) {}

    void Request(HttpRequest request, std::function<void(const HttpResult&)> cb) override {
        requester_.Request(std::move(request), std::move(cb));
    }

private:
    AsioRequester requester_;
};
#endif

// Requester implementation
Requester::Requester(net::any_io_executor ctx, TlsOptions const& tls_options) {
#ifdef LD_CURL_NETWORKING
    impl_ = std::make_unique<CurlRequesterImpl>(ctx, tls_options);
#else
    impl_ = std::make_unique<AsioRequesterImpl>(ctx, tls_options);
#endif
}

Requester::~Requester() = default;

Requester::Requester(Requester&&) noexcept = default;
Requester& Requester::operator=(Requester&&) noexcept = default;

void Requester::Request(HttpRequest request, std::function<void(const HttpResult&)> cb) {
    impl_->Request(std::move(request), std::move(cb));
}

} // namespace launchdarkly::network
