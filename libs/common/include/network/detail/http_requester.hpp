#pragma once

#include <cstdint>
#include <future>
#include <map>
#include <ostream>
#include <string>

#include "config/detail/built/http_properties.hpp"

namespace launchdarkly::network::detail {

class HttpResult {
   public:
    using StatusCode = uint64_t;
    using HeadersType = std::map<std::string, std::string>;
    using BodyType = std::optional<std::string>;

    bool IsError() const;

    std::optional<std::string> const& ErrorMessage() const;

    StatusCode Status() const;

    BodyType const& Body() const;

    HeadersType const& Headers() const;

    HttpResult(StatusCode status, BodyType body, HeadersType headers);

    HttpResult(std::optional<std::string> error_message);

    friend std::ostream& operator<<(std::ostream& out, HttpResult const& res) {
        if (res.error_) {
            out << "Error("
                << (res.error_message_.has_value() ? res.error_message_.value()
                                                   : "unknown");
        } else {
            out << "Success(" << res.status_;
            if (res.body_) {
                out << "," << res.body_.value();
            }
            if (!res.headers_.empty()) {
                // TODO: Headers
            }
        }
        out << ")";

        return out;
    }

   private:
    bool error_;
    std::optional<std::string> error_message_;
    StatusCode status_;
    BodyType body_;
    HeadersType headers_;
};

enum class HttpMethod { kPost, kGet, kReport, kPut };

class HttpRequest {
   public:
    using HeadersType = std::map<std::string, std::string>;
    using BodyType = std::optional<std::string>;

    HttpMethod Method() const;
    BodyType const& Body() const;
    config::detail::built::HttpProperties const& Properties() const;
    std::string const& Host() const;
    std::string const& Port() const;
    std::string const& Path() const;
    bool Https() const;

    HttpRequest(std::string url,
                HttpMethod method,
                config::detail::built::HttpProperties properties,
                BodyType body);

   private:
    HttpMethod method_;
    std::optional<std::string> body_;
    config::detail::built::HttpProperties properties_;
    std::string host_;
    std::string port_;
    std::string path_;
    bool is_https_;
};

//class IRequestState {
//   public:
//    virtual void run() = 0;
//
//    virtual ~IRequestState() = default;
//    IRequestState(IRequestState const& item) = delete;
//    IRequestState(IRequestState&& item) = delete;
//    IRequestState& operator=(IRequestState const&) = delete;
//    IRequestState& operator=(IRequestState&&) = delete;
//
//   protected:
//    IRequestState() = default;
//};

//class IHttpRequester {
//   public:
//    using ResponseHandler = std::function<void(HttpResult result)>;
//    virtual std::shared_ptr<IRequestState> Request(HttpRequest request,
//                                                   ResponseHandler handler) = 0;
//
//    virtual ~IHttpRequester() = default;
//    IHttpRequester(IHttpRequester const& item) = delete;
//    IHttpRequester(IHttpRequester&& item) = delete;
//    IHttpRequester& operator=(IHttpRequester const&) = delete;
//    IHttpRequester& operator=(IHttpRequester&&) = delete;
//
//   protected:
//    IHttpRequester() = default;
//};

}  // namespace launchdarkly::network::detail