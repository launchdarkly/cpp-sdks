#pragma once

#include "network/detail/http_requester.hpp"

#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
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
    Derived& derived() { return static_cast<Derived&>(*this); }
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

    void fail(beast::error_code ec, char const* what) {
        handler_(HttpResult(std::string(what) + ": " + ec.message()));
    }

    void do_resolve() {
        resolver_.async_resolve(
            host_, port_,
            beast::bind_front_handler(&Session::on_resolve,
                                      derived().shared_from_this()));
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec)
            return fail(ec, "resolve");

        beast::get_lowest_layer(derived().stream())
            .expires_after(connect_timeout_);

        beast::get_lowest_layer(derived().stream())
            .async_connect(results, beast::bind_front_handler(
                                        &Session::on_connect,
                                        derived().shared_from_this()));
    }

    void on_connect(beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type eps) {
        if (ec) {
            return fail(ec, "connect");
        }

        derived().do_handshake();
    }

    void on_handshake(beast::error_code ec) {
        if (ec)
            return fail(ec, "handshake");

        do_write();
    }

    void do_write() {
        beast::get_lowest_layer(derived().stream())
            .expires_after(response_timeout_);

        http::async_write(
            derived().stream(), req_,
            beast::bind_front_handler(&Session::on_write,
                                      derived().shared_from_this()));
    }

    void on_write(beast::error_code ec, std::size_t) {
        if (ec)
            return fail(ec, "write");

        //        logger_("reading response");

        http::async_read_some(
            derived().stream(), buffer_, parser_,
            beast::bind_front_handler(&Session::on_read,
                                      derived().shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec) {
            return fail(ec, "read");
        }
        if (parser_.is_done()) {
            handler_(HttpResult(parser_.get().result_int(),
                                parser_.get().body(),
                                HttpResult::HeadersType()));
            // TODO: Shutdown.
            return;
        }

        beast::get_lowest_layer(derived().stream())
            .expires_after(read_timeout_);

        http::async_read_some(
            derived().stream(), buffer_, parser_,
            beast::bind_front_handler(&Session::on_read,
                                      derived().shared_from_this()));
    }

    void do_close() { beast::get_lowest_layer(derived().stream()).cancel(); }
};

class PlaintextClient : public Session<PlaintextClient>,
                        public std::enable_shared_from_this<PlaintextClient> {
   public:
    using ResponseHandler = std::function<void(HttpResult result)>;

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

    void do_handshake() {
        // No handshake for plaintext; immediately send the request instead.
        do_write();
    }

    void run() { do_resolve(); }

    beast::tcp_stream& stream() { return stream_; }

   private:
    beast::tcp_stream stream_;
};

class AsioRequester {
   public:
    AsioRequester(net::any_io_executor ctx) : ctx_(ctx) {}

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
            // TODO: Determine http/https.
            http::request<http::string_body> beast_request;
            beast_request.method(http::verb::get);  // TODO: Get from request.
            beast_request.body() = "";
            beast_request.target(request.Path());
            beast_request.prepare_payload();
            beast_request.set(http::field::host, request.Host());

            // TODO: Transcribe headers.

            auto& properties = request.Properties();
            std::make_shared<PlaintextClient>(
                strand, beast_request, request.Host(), request.Port(),
                properties.ConnectTimeout(), properties.ResponseTimeout(),
                properties.ReadTimeout(), std::move(handler))
                ->run();
        });

        return result.get();
    }

   private:
    net::any_io_executor ctx_;
};

}  // namespace launchdarkly::network::detail
