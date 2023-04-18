#include <boost/url/parse.hpp>

#include "network/detail/http_requester.hpp"

namespace launchdarkly::network::detail {

HttpResult::StatusCode HttpResult::Status() const {
    return status_;
}

HttpResult::BodyType const& HttpResult::Body() const {
    return body_;
}

HttpResult::HeadersType const& HttpResult::Headers() const {
    return headers_;
}

HttpResult::HttpResult(HttpResult::StatusCode status,
                       std::optional<std::string> body,
                       HttpResult::HeadersType headers)
    : status_(status),
      body_(std::move(body)),
      headers_(std::move(headers)),
      error_(false) {}

bool HttpResult::IsError() const {
    return error_;
}

std::optional<std::string> const& HttpResult::ErrorMessage() const {
    return error_message_;
}

HttpResult::HttpResult(std::optional<std::string> error_message)
    : error_message_(error_message), error_(true) {}

HttpMethod HttpRequest::Method() const {
    return method_;
}

HttpRequest::BodyType const& HttpRequest::Body() const {
    return body_;
}

config::detail::HttpProperties const& HttpRequest::Properties() const {
    return properties_;
}

std::string const& HttpRequest::Host() const {
    return host_;
}

std::string const& HttpRequest::Path() const {
    return path_;
}

HttpRequest::HttpRequest(std::string url,
                         HttpMethod method,
                         config::detail::HttpProperties properties,
                         BodyType body)
    : properties_(properties), method_(method), body_(body) {
    auto uri_components = boost::urls::parse_uri(url);

    host_ = uri_components->host();
    path_ = uri_components->path();

    is_https_ = uri_components->scheme_id() == boost::urls::scheme::https;
    if (uri_components->has_port()) {
        port_ = uri_components->port();
    } else {
        port_ = is_https_ ? "443" : "80";
    }
}

std::string const& HttpRequest::Port() const {
    return port_;
}
bool HttpRequest::Https() const {
    return is_https_;
}

}  // namespace launchdarkly::network::detail
