#pragma once

#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <cstdint>
#include <future>
#include <map>
#include <ostream>
#include <string>

#include "config/detail/built/http_properties.hpp"

namespace launchdarkly::network::detail {

struct CaseInsensitiveComparator {
    bool operator()(std::string const& lhs,
                    std::string const& rhs) const noexcept {
        return ::strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
    }
};

class HttpResult {
   public:
    using StatusCode = uint64_t;
    using HeadersType =
        std::map<std::string, std::string, CaseInsensitiveComparator>;
    using BodyType = std::optional<std::string>;

    [[nodiscard]] bool IsError() const;

    [[nodiscard]] std::optional<std::string> const& ErrorMessage() const;

    [[nodiscard]] StatusCode Status() const;

    [[nodiscard]] BodyType const& Body() const;

    [[nodiscard]] HeadersType const& Headers() const;

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
                for (auto const& pair : res.headers_) {
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
    using BodyType = std::optional<std::string>;

    [[nodiscard]] HttpMethod Method() const;
    [[nodiscard]] BodyType const& Body() const;
    [[nodiscard]] config::detail::built::HttpProperties const& Properties()
        const;
    [[nodiscard]] std::string const& Host() const;
    [[nodiscard]] std::string const& Port() const;
    [[nodiscard]] std::string const& Path() const;

    [[nodiscard]] std::string const& Url() const;

    [[nodiscard]] bool Https() const;

    /**
     * Indicates if a request is valid. Meaning that it has correctly formed
     * data that can be used to make an http request.
     *
     * @return True if the request is valid.
     */
    [[nodiscard]] bool Valid() const;

    HttpRequest(std::string const& url,
                HttpMethod method,
                config::detail::built::HttpProperties properties,
                BodyType body);

    /**
     * Move the contents of the base request and create a new request
     * incorporating the provided properties.
     * @param base_request The base request.
     * @param properties The properties for the request.
     */
    HttpRequest(HttpRequest& base_request,
                config::detail::built::HttpProperties properties);

   private:
    std::string url_;
    HttpMethod method_;
    std::optional<std::string> body_;
    config::detail::built::HttpProperties properties_;
    std::string host_;
    std::string port_;
    std::string path_;
    std::map<std::string, std::string> params_;
    bool is_https_;
    bool valid_;
};

bool IsRecoverableStatus(HttpResult::StatusCode status);

/**
 * Append a path to a URL. This will account for query parameters on the
 * original URL. This will also normalize the URL.
 *
 * If the input URL doesn't parse, then std::nullopt will be returned.
 *
 * @param url_in Input URL, if std::nullopt, the method will return
 * std::nullopt. This is to facilitate multiple appends without having to check
 * intermediate results.
 *
 * @param to_append Path to append to the URL.
 * @return The appended URL, or std::nullopt if the URL could not be parsed.
 */
std::optional<std::string> AppendUrl(std::optional<std::string> url_in,
                                     std::string const& to_append);

}  // namespace launchdarkly::network::detail
