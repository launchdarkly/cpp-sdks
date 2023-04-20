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
   private:
    Derived& derived() { return static_cast<Derived&>(*this); }
    http::request<http::string_body> req_;
    //    std::chrono::milliseconds connect_timeout_;
    //    std::chrono::milliseconds response_timeout_;
    //    std::optional<std::chrono::milliseconds> read_timeout_;

   protected:
    beast::flat_buffer buffer_;
    std::string host_;
    std::string port_;
    tcp::resolver resolver_;
    http::response_parser<http::string_body> parser_;
    IHttpRequester::ResponseHandler handler_;

    using cb = std::function<void(IHttpRequester::ResponseHandler handler)>;
    using body = http::string_body;

   public:
    Session(net::any_io_executor const& exec,
            std::string host,
            std::string port,
            http::request<http::string_body> req,
            //            std::chrono::milliseconds connect_timeout,
            //            std::chrono::milliseconds response_timeout,
            //            std::optional<std::chrono::milliseconds> read_timeout,
            IHttpRequester::ResponseHandler handler)
        : req_(std::move(req)),
          resolver_(exec),
          host_(std::move(host)),
          port_(std::move(port)),
          handler_(std::move(handler)) {
        //        parser_.get().body().on_event(std::move(receiver));
        parser_.get();
    }

    void fail(beast::error_code ec, char const* what) {
        handler_(HttpResult(std::string(what) + ": " + ec.message()));
        //        logger_(std::string(what) + ": " + ec.message());
    }

    void do_resolve() {
        //        logger_("resolving " + host_ + ":" + port_);
        resolver_.async_resolve(
            host_, port_,
            beast::bind_front_handler(&Session::on_resolve,
                                      derived().shared_from_this()));
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec)
            return fail(ec, "resolve");

        //        logger_("connecting (" +
        //        std::to_string(connect_timeout_.count()) +
        //                " sec timeout)");

        //        beast::get_lowest_layer(derived().stream())
        //            .expires_after(connect_timeout_);

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
        //        logger_("making request (" +
        //        std::to_string(response_timeout_.count()) +
        //                " sec timeout)");

        //        beast::get_lowest_layer(derived().stream())
        //            .expires_after(response_timeout_);

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

        //        if (read_timeout_) {
        //            beast::get_lowest_layer(derived().stream())
        //                .expires_after(*read_timeout_);
        //        } else {
        beast::get_lowest_layer(derived().stream()).expires_never();
        //        };

        http::async_read_some(
            derived().stream(), buffer_, parser_,
            beast::bind_front_handler(&Session::on_read,
                                      derived().shared_from_this()));
    }

    void do_close() {
        //        logger_("closing");
        beast::get_lowest_layer(derived().stream()).cancel();
    }
};

class PlaintextClient : public IRequestState,
                        public Session<PlaintextClient>,
                        public std::enable_shared_from_this<PlaintextClient> {
   public:
    PlaintextClient(
        net::any_io_executor ex,
        http::request<http::string_body> req,
        std::string host,
        std::string port,
        //                    std::optional<std::chrono::milliseconds>
        //                    read_timeout,
        IHttpRequester::ResponseHandler handler)
        : Session<PlaintextClient>(
              ex,
              std::move(host),
              std::move(port),
              std::move(req),
              //                                   read_timeout,
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

class AsioRequester : public IHttpRequester {
   public:
    AsioRequester(net::any_io_executor ctx) : ctx_(ctx) {}

    template <typename CompletionToken>
    typename boost::asio::async_result<CompletionToken,
                                       void(HttpResult result)>::return_type
    Request(boost::asio::io_service& ios,
            HttpRequest request,
            CompletionToken&& token) {
        namespace asio = boost::asio;
        namespace system = boost::system;

        using Sig = void(HttpResult result);
        using Result = asio::async_result<CompletionToken, Sig>;
        using Handler = typename Result::completion_handler_type;

        Handler handler(std::forward<decltype(token)>(token));
        Result result(handler);

        ios.post([handler, request]() mutable {
            handler(system::error_code(), request);
        });

        return result.get();
    }

    std::shared_ptr<IRequestState> Request(HttpRequest request,
                                           ResponseHandler handler) override {
        // TODO: Determine http/https.
        http::request<http::string_body> beast_request;
        beast_request.method(http::verb::get);  // TODO: Get from request.
        beast_request.body() = "";
        beast_request.target(request.Path());
        beast_request.prepare_payload();
        beast_request.set(http::field::host, request.Host());
        //        PlaintextClient plain()

        return std::make_shared<PlaintextClient>(
            net::make_strand(ctx_), beast_request, request.Host(),
            request.Port(), std::move(handler));
    }

   private:
    net::any_io_executor ctx_;
};

}  // namespace launchdarkly::network::detail
