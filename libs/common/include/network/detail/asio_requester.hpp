#pragma once

#include "network/detail/http_requester.hpp"

#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>

#include <boost/certify/extensions.hpp>
#include <boost/certify/https_verification.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

namespace launchdarkly::network::detail {

template <class Derived>
class
    Session {  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
   public:
    using ResponseHandler = std::function<void(HttpResult result)>;

   private:
    Derived& GetDerived() { return static_cast<Derived&>(*this); }
    http::request<http::string_body> req_;
    std::chrono::milliseconds connect_timeout_;
    std::chrono::milliseconds response_timeout_;
    std::chrono::milliseconds read_timeout_;

   protected:
    beast::flat_buffer buffer_;
    std::string host_;
    std::string port_;
    tcp::resolver resolver_;
    http::response_parser<http::string_body> parser_;
    ResponseHandler handler_;

    using cb = std::function<void(ResponseHandler handler)>;
    using body = http::string_body;

   public:
    Session(net::any_io_executor const& exec,
            std::string host,
            std::string port,
            http::request<http::string_body> req,
            std::chrono::milliseconds connect_timeout,
            std::chrono::milliseconds response_timeout,
            std::chrono::milliseconds read_timeout,
            ResponseHandler handler)
        : req_(std::move(req)),
          resolver_(exec),
          host_(std::move(host)),
          port_(std::move(port)),
          connect_timeout_(connect_timeout),
          response_timeout_(response_timeout),
          read_timeout_(read_timeout),
          handler_(std::move(handler)) {
        parser_.get();
    }

    void Fail(beast::error_code ec, char const* what) {
        // TODO: Is it safe to cancel this if it has already failed?
        beast::get_lowest_layer(GetDerived().Stream()).cancel();
        handler_(HttpResult(std::string(what) + ": " + ec.message()));
    }

    void DoResolve() {
        resolver_.async_resolve(
            host_, port_,
            beast::bind_front_handler(&Session::OnResolve,
                                      GetDerived().shared_from_this()));
    }

    void OnResolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec)
            return Fail(ec, "resolve");

        beast::get_lowest_layer(GetDerived().Stream())
            .expires_after(connect_timeout_);

        beast::get_lowest_layer(GetDerived().Stream())
            .async_connect(results, beast::bind_front_handler(
                                        &Session::OnConnect,
                                        GetDerived().shared_from_this()));
    }

    void OnConnect(beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type eps) {

        if (ec) {
            return Fail(ec, "connect");
        }

        GetDerived().DoHandshake();
    }

    void OnHandshake(beast::error_code ec) {
        if (ec)
            return Fail(ec, "handshake");

        DoWrite();
    }

    void DoWrite() {
        beast::get_lowest_layer(GetDerived().Stream())
            .expires_after(response_timeout_);

        http::async_write(GetDerived().Stream(), req_,
            beast::bind_front_handler(
                              &Session::OnWrite,
                              GetDerived().shared_from_this()));
    }

    void OnWrite(beast::error_code ec, std::size_t) {
        if (ec)
            return Fail(ec, "write");

        //        logger_("reading response");

        http::async_read_some(
            GetDerived().Stream(), buffer_, parser_,
            beast::bind_front_handler(&Session::OnRead,
                                      GetDerived().shared_from_this()));
    }

    void OnRead(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec) {
            return Fail(ec, "read");
        }
        if (parser_.is_done()) {
            DoClose();
            handler_(HttpResult(parser_.get().result_int(),
                                parser_.get().body(),
                                HttpResult::HeadersType()));
            return;
        }

        beast::get_lowest_layer(GetDerived().Stream())
            .expires_after(read_timeout_);

        http::async_read_some(
            GetDerived().Stream(), buffer_, parser_,
            beast::bind_front_handler(&Session::OnRead,
                                      GetDerived().shared_from_this()));
    }

    void DoClose() { beast::get_lowest_layer(GetDerived().Stream()).cancel(); }
};

class PlaintextClient : public Session<PlaintextClient>,
                        public std::enable_shared_from_this<PlaintextClient> {
   public:
    using ResponseHandler = Session<PlaintextClient>::ResponseHandler;

    PlaintextClient(net::any_io_executor ex,
                    http::request<http::string_body> req,
                    std::string host,
                    std::string port,
                    std::chrono::milliseconds connect_timeout,
                    std::chrono::milliseconds response_timeout,
                    std::chrono::milliseconds read_timeout,
                    ResponseHandler handler)
        : Session<PlaintextClient>(ex,
                                   std::move(host),
                                   std::move(port),
                                   std::move(req),
                                   connect_timeout,
                                   response_timeout,
                                   read_timeout,
                                   std::move(handler)),
          stream_{ex} {}

    void DoHandshake() {
        // No handshake for plaintext; immediately send the request instead.
        DoWrite();
    }

    void Run() { DoResolve(); }

    beast::tcp_stream& Stream() { return stream_; }

   private:
    beast::tcp_stream stream_;
};

class EncryptedClient : public Session<EncryptedClient>,
                        public std::enable_shared_from_this<EncryptedClient> {
   public:
    using ResponseHandler = Session<EncryptedClient>::ResponseHandler;

    EncryptedClient(net::any_io_executor ex,
                    std::shared_ptr<ssl::context> ssl_ctx,
                    http::request<http::string_body> req,
                    std::string host,
                    std::string port,
                    std::chrono::milliseconds connect_timeout,
                    std::chrono::milliseconds response_timeout,
                    std::chrono::milliseconds read_timeout,
                    ResponseHandler handler)
        : Session<EncryptedClient>(ex,
                                   std::move(host),
                                   std::move(port),
                                   std::move(req),
                                   connect_timeout,
                                   response_timeout,
                                   read_timeout,
                                   std::move(handler)),
          ssl_ctx_(ssl_ctx),
          stream_{ex, *ssl_ctx} {}

    virtual void Run() {
        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(stream_.native_handle(), host_.c_str())) {
            beast::error_code ec{static_cast<int>(::ERR_get_error()),
                                 net::error::get_ssl_category()};

            DoClose();
            // TODO: Should this be treated as a terminal error for the request.
            handler_(HttpResult("failed to set TLS host name extension: " +
                                ec.message()));
            return;
        }

        DoResolve();
    }

    void DoHandshake() {
        stream_.async_handshake(ssl::stream_base::client,
                                beast::bind_front_handler(&EncryptedClient::OnHandshake, shared()));
    }

    beast::ssl_stream<beast::tcp_stream>& Stream() { return stream_; }

   private:
    std::shared_ptr<ssl::context> ssl_ctx_;
    beast::ssl_stream<beast::tcp_stream> stream_;

    std::shared_ptr<EncryptedClient> shared() {
        return std::static_pointer_cast<EncryptedClient>(shared_from_this());
    }
};

class AsioRequester {
    /**
     * Each request is currently its own strand. If the TCP Stream was to be
     * re-used between requests, then each request to the same Stream would
     * need to be using the same strand.
     *
     * If this code is refactored to allow re-use of TCP streams, then that
     * must be accounted for.
     */
   public:
    AsioRequester(net::any_io_executor ctx)
        : ctx_(ctx),
          ssl_ctx_(
              std::make_shared<ssl::context>(ssl::context::tlsv12_client)) {
        ssl_ctx_->set_verify_mode(ssl::verify_peer |
                                  ssl::verify_fail_if_no_peer_cert);

        ssl_ctx_->set_default_verify_paths();
        boost::certify::enable_native_https_server_verification(*ssl_ctx_);
    }

    template <typename CompletionToken>
    auto Request(HttpRequest request, CompletionToken&& token) {
        namespace asio = boost::asio;
        namespace system = boost::system;

        using Sig = void(HttpResult result);
        using Result = asio::async_result<std::decay_t<CompletionToken>, Sig>;
        using Handler = typename Result::completion_handler_type;

        Handler handler(std::forward<decltype(token)>(token));
        Result result(handler);

        auto strand = net::make_strand(ctx_);
        boost::asio::post(strand, [strand, handler, request, this]() mutable {
            http::request<http::string_body> beast_request;
            beast_request.method(http::verb::get);  // TODO: Get from request.
            beast_request.body() = "";
            beast_request.target(request.Path());
            beast_request.prepare_payload();
            beast_request.set(http::field::host, request.Host());

            // TODO: Transcribe headers.

            auto& properties = request.Properties();
            if (request.Https()) {
                std::make_shared<EncryptedClient>(
                    strand, ssl_ctx_, beast_request, request.Host(),
                    request.Port(), properties.ConnectTimeout(),
                    properties.ResponseTimeout(), properties.ReadTimeout(),
                    std::move(handler))
                    ->Run();
            } else {
                std::make_shared<PlaintextClient>(
                    strand, beast_request, request.Host(), request.Port(),
                    properties.ConnectTimeout(), properties.ResponseTimeout(),
                    properties.ReadTimeout(), std::move(handler))
                    ->Run();
            }
        });

        return result.get();
    }

   private:
    net::any_io_executor ctx_;
    /**
     * The SSL context is a shared pointer to reduce the complexity of the
     * relationship between a requests lifetime and the lifetime of the
     * requester.
     */
    std::shared_ptr<ssl::context> ssl_ctx_;
};

}  // namespace launchdarkly::network::detail
