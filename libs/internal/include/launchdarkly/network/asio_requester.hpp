#pragma once

#include "http_requester.hpp"

#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/detail/unreachable.hpp>

#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/core/ignore_unused.hpp>

#include "foxy/client_session.hpp"

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

namespace launchdarkly::network {

using VerifyMode = enum config::shared::built::TlsOptions::VerifyMode;

static unsigned char const kRedirectLimit = 20;

static bool IsAbsolute(std::string_view str) {
    return str.find("://") != std::string::npos || str.find("//") == 0;
}

static bool NeedsRedirect(HttpResult const& res) {
    // 300, multiple choices. Not actionable.
    // 302, found, but not available for unforeseen reasons is not actionable.
    // 303, attempting to change the method. We only want the original method.
    // Redirect from a PUT to a GET for instance.
    // 304, for use with etags, needs to be handled by the caller.
    // 307, same as 303, but for non-GET operations.
    return res.Status() == 301 ||
           res.Status() == 308 && res.Headers().count("location") != 0;
}

/**
 * Converts the given HttpMethod to a boost beast HTTP verb.
 * If the verb is unrecognized, returns http::verb::get.
 */
static http::verb ConvertMethod(HttpMethod method) {
    switch (method) {
        case HttpMethod::kPost:
            return http::verb::post;
        case HttpMethod::kGet:
            return http::verb::get;
        case HttpMethod::kReport:
            return http::verb::report;
        case HttpMethod::kPut:
            return http::verb::put;
    }
    launchdarkly::detail::unreachable();
}

static http::request<http::string_body> MakeBeastRequest(
    HttpRequest const& request) {
    http::request<http::string_body> beast_request;

    beast_request.method(ConvertMethod(request.Method()));
    auto body = request.Body();
    if (body) {
        beast_request.body() = body.value();
    } else {
        beast_request.body() = "";
    }
    if (request.Path().empty()) {
        beast_request.target("/");
    } else {
        beast_request.target(request.Path());
    }

    beast_request.prepare_payload();
    beast_request.set(http::field::host, request.Host());

    auto const& properties = request.Properties();

    for (auto const& pair : request.Properties().BaseHeaders()) {
        beast_request.set(pair.first, pair.second);
    }

    return beast_request;
}

static std::optional<HttpRequest> MakeRedirectRequest(HttpRequest const& req,
                                                      HttpResult const& res) {
    auto location = res.Headers().find("location");
    // Location should be verified to be present before attempting to
    // make the redirect request.
    assert(location != res.Headers().end());
    // Start the request over with the new URL.
    if (IsAbsolute(location->second)) {
        return HttpRequest(location->second, req.Method(), req.Properties(),
                           req.Body());
    }
    auto new_url = AppendUrl(req.Url(), location->second);
    if (new_url) {
        return HttpRequest(*new_url, req.Method(), req.Properties(),
                           req.Body());
    }

    return std::nullopt;
}

static boost::optional<net::ssl::context&> ToOptRef(
    net::ssl::context* maybe_val) {
    if (maybe_val) {
        return *maybe_val;
    }
    return boost::none;
}

class FoxyClient
    : public std::enable_shared_from_this<
          FoxyClient> {  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
   public:
    using ResponseHandler = std::function<void(HttpResult result)>;

   private:
    std::shared_ptr<net::ssl::context> ssl_context_;
    std::string host_;
    std::string port_;
    http::request<http::string_body> req_;
    std::chrono::milliseconds connect_timeout_;
    std::chrono::milliseconds response_timeout_;
    ResponseHandler handler_;
    http::response<http::string_body> resp_;
    launchdarkly::foxy::client_session session_;

   public:
    FoxyClient(net::any_io_executor const& exec,
               std::shared_ptr<net::ssl::context> ssl_context,
               std::string host,
               std::string port,
               http::request<http::string_body> req,
               std::chrono::milliseconds connect_timeout,
               std::chrono::milliseconds response_timeout,
               ResponseHandler handler)
        : ssl_context_(std::move(ssl_context)),
          host_(std::move(host)),
          port_(std::move(port)),
          req_(std::move(req)),
          connect_timeout_(connect_timeout),
          response_timeout_(response_timeout),
          handler_(std::move(handler)),
          session_(exec,
                   launchdarkly::foxy::session_opts{
                       ToOptRef(ssl_context_.get()), connect_timeout_}),
          resp_() {}

    void Run() {
        session_.async_connect(host_, port_,
                               beast::bind_front_handler(&FoxyClient::OnConnect,
                                                         shared_from_this()));
    }

    void OnConnect(boost::system::error_code ec) {
        if (ec) {
            return Fail(ec, "connect");
        }
        session_.opts.timeout = response_timeout_;
        session_.async_request(
            req_, resp_,
            beast::bind_front_handler(&FoxyClient::OnResponse,
                                      shared_from_this()));
    }

    void OnResponse(boost::system::error_code ec) {
        if (ec) {
            return Fail(ec, "request");
        }
        handler_(MakeResult());
        session_.async_shutdown(beast::bind_front_handler(
            &FoxyClient::OnShutdown, shared_from_this()));
    }

    void Fail(beast::error_code ec, char const* what) {
        std::string error_string = std::string(what) + ": " + ec.message();
        handler_(HttpResult(error_string));
        session_.async_shutdown(beast::bind_front_handler(
            &FoxyClient::OnShutdown, shared_from_this()));
    }

    void OnShutdown(boost::system::error_code ec) { boost::ignore_unused(ec); }

    /**
     * Produce an HttpResult from the parser_. Should be called if the parser
     * is done.
     * @return The created HttpResult.
     */
    [[nodiscard]] HttpResult MakeResult() const {
        auto headers = HttpResult::HeadersType();
        for (auto const& field : resp_.base()) {
            headers.insert_or_assign(field.name_string(), field.value());
        }
        auto result =
            HttpResult(resp_.result_int(), std::make_optional(resp_.body()),
                       std::move(headers));
        return result;
    }
};

/**
 * Class which allows making requests using boost::beast. A requester should be
 * created once, and then used for many requests. This class supports both
 * http and https requests.
 *
 * Requests complete asynchronously and use completion tokens. This allows
 * for the request to be handled with futures or callbacks. If C++20 was in
 * use, then coroutines would also be usable.
 *
 * In these examples HttpProperties are built, but in most SDK use cases
 * the properties should be used from the built configuration object.
 *
 * Futures:
 * ```
 * auto res =
 * requester
 *   .Request(HttpRequest("http://localhost:8080",
 *   HttpMethod::kGet,
 *                        HttpPropertiesBuilder<ClientSDK>().Build(),
 *                        std::nullopt),
 *            boost::asio::use_future)
 *   .get();
 * ```
 *
 * Callbacks:
 * requester.Request(
 * HttpRequest("http://localhost:8080/", HttpMethod::kGet,
 *             HttpPropertiesBuilder<ClientSDK>().Build(),
 *             std::nullopt),
 * [](auto response) {
 *     std::cout << "Response1: " << response << std::endl;
 * });
 * ```
 */
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
    AsioRequester(net::any_io_executor ctx, VerifyMode verify_mode)
        : ctx_(std::move(ctx)),
          ssl_ctx_(std::make_shared<net::ssl::context>(
              launchdarkly::foxy::make_ssl_ctx(ssl::context::tlsv12_client))) {
        ssl_ctx_->set_default_verify_paths();
        ssl_ctx_->set_verify_mode(verify_mode == VerifyMode::kVerifyPeer
                                      ? ssl::verify_peer
                                      : ssl::verify_none);
    }

    template <typename CompletionToken>
    auto Request(HttpRequest request, CompletionToken&& token) {
        // TODO: Clang-tidy wants to pass the request by reference, but I am not
        // confident that lifetime would make sense.

        namespace asio = boost::asio;
        namespace system = boost::system;

        using Sig = void(HttpResult result);
        using Result = asio::async_result<std::decay_t<CompletionToken>, Sig>;
        using Handler = typename Result::completion_handler_type;

        Handler handler(std::forward<decltype(token)>(token));
        Result result(handler);

        InnerRequest(net::make_strand(ctx_), request, std::move(handler), 0);

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

    void InnerRequest(boost::asio::any_io_executor exec,
                      std::optional<HttpRequest> request,
                      std::function<void(HttpResult)> callback,
                      unsigned char redirect_count) {
        if (redirect_count > kRedirectLimit) {
            boost::asio::post(exec, [callback, request]() mutable {
                callback(
                    HttpResult("Redirects exceeded 20, cancelling request."));
            });
            return;
        }

        // The request is invalid and cannot be made, so produce an error
        // result.
        if (!request || !request->Valid()) {
            boost::asio::post(exec, [callback, request]() mutable {
                callback(HttpResult(
                    "The request was malformed and could not be made."));
            });
            return;
        }

        boost::asio::post(exec, [exec, callback, request, this,
                                 redirect_count]() mutable {
            auto beast_request = MakeBeastRequest(*request);

            const auto& properties = request->Properties();

            std::string service =
                request->Port().value_or(request->Https() ? "https" : "http");

            std::shared_ptr<ssl::context> ssl;
            if (request->Https()) {
                ssl = this->ssl_ctx_;
            }

            std::make_shared<FoxyClient>(
                exec, std::move(ssl), request->Host(), service, beast_request,
                properties.ConnectTimeout(), properties.ResponseTimeout(),
                [exec, callback, request, this, redirect_count](auto res) {
                    NeedsRedirect(res)
                        ? InnerRequest(exec, MakeRedirectRequest(*request, res),
                                       callback, redirect_count + 1)
                        : callback(res);
                })
                ->Run();
        });
    }
};

}  // namespace launchdarkly::network
