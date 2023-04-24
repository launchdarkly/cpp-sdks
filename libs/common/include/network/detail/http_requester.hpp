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
                out << ", " << res.body_.value();
            }
            if (!res.headers_.empty()) {
                out << ", {";
                bool first = true;
                for (auto& pair : res.headers_) {
                    if (first) {
                        first = false;
                    } else {
                        out << ", ";
                    }
                    out << pair.first << " : " << pair.second;
                }
                out << "}";
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
    std::map<std::string, std::string> const& Params() const;
    bool Https() const;

    HttpRequest(std::string const& url,
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
    std::map<std::string, std::string> params_;
    bool is_https_;
};

}  // namespace launchdarkly::network::detail
