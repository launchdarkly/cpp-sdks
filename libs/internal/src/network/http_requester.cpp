#include <boost/url.hpp>

#include <cstring>
#include <optional>
#include <utility>

#include <launchdarkly/network/http_requester.hpp>

namespace launchdarkly::network {

bool CaseInsensitiveComparator::operator()(
    std::string const& lhs,
    std::string const& rhs) const noexcept {
#ifdef _MSC_VER
    return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
#else
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
#endif
}

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
    : error_(false),
      status_(status),
      body_(std::move(body)),
      headers_(std::move(headers)) {}

bool HttpResult::IsError() const {
    return error_;
}

std::optional<std::string> const& HttpResult::ErrorMessage() const {
    return error_message_;
}

HttpResult::HttpResult(std::optional<std::string> error_message)
    : error_(true), error_message_(std::move(error_message)), status_(0) {}

HttpMethod HttpRequest::Method() const {
    return method_;
}

HttpRequest::BodyType const& HttpRequest::Body() const {
    return body_;
}

config::shared::built::HttpProperties const& HttpRequest::Properties() const {
    return properties_;
}

std::string const& HttpRequest::Host() const {
    return host_;
}

std::string const& HttpRequest::Path() const {
    return path_;
}

std::string const& HttpRequest::Url() const {
    return url_;
}

std::optional<std::string> const& HttpRequest::HttpProxyHost() const {
    return http_proxy_host_;
}

std::optional<std::string> const& HttpRequest::HttpProxyPort() const {
    return http_proxy_port_;
}

// What I'm doing: supporting HTTP proxy in the AsioEventRequestor.
// It's annoying that we have usage in SSE client but also here that both needs
// to be updated.

HttpRequest::HttpRequest(std::string const& url,
                         HttpMethod method,
                         config::shared::built::HttpProperties properties,
                         HttpRequest::BodyType body)
    : url_(url),
      method_(method),
      body_(std::move(body)),
      properties_(std::move(properties)),
      port_(std::nullopt) {
    auto uri_components = boost::urls::parse_uri(url);

    // If the URI cannot be parsed, then the request is not valid.
    if (!uri_components) {
        valid_ = false;
        return;
    }

    boost::urls::url boost_url = uri_components.value();
    // Make paths absolute and slashes consistent.
    boost_url.normalize();

    host_ = uri_components->host();
    // The c_str here is to remove extra nulls from normalizing the path.
    // Clang calls this redundant, but it is very much required.
    path_ =
        boost_url.path().c_str();  // NOLINT(readability-redundant-string-cstr)
    if (!boost_url.query().empty()) {
        // For a boost beast request we need the query string in the path.
        path_ = path_ + "?" + uri_components->query();
    }

    is_https_ = uri_components->scheme_id() == boost::urls::scheme::https;
    if (uri_components->has_port()) {
        port_ = uri_components->port();
    }

    if (properties_.HttpProxy()) {
        auto const http_proxy = properties_.HttpProxy().value();
        auto http_proxy_uri_components = boost::urls::parse_uri(http_proxy);
        if (!http_proxy_uri_components) {
            valid_ = false;
            return;
        }

        http_proxy_host_ = http_proxy_uri_components->host();
        if (http_proxy_uri_components->has_port()) {
            http_proxy_port_ = http_proxy_uri_components->port();
        }
    }

    valid_ = true;
}

HttpRequest::HttpRequest(HttpRequest& base_request,
                         config::shared::built::HttpProperties properties)
    : url_(base_request.url_),
      method_(base_request.method_),
      body_(std::move(base_request.body_)),
      properties_(std::move(properties)),
      host_(base_request.host_),
      port_(base_request.port_),
      path_(base_request.path_),
      http_proxy_host_(base_request.http_proxy_host_),
      http_proxy_port_(base_request.http_proxy_port_),
      is_https_(base_request.is_https_),
      valid_(base_request.valid_) {}

std::optional<std::string> const& HttpRequest::Port() const {
    return port_;
}
bool HttpRequest::Https() const {
    return is_https_;
}

bool HttpRequest::Valid() const {
    return valid_;
}

bool IsRecoverableStatus(HttpResult::StatusCode status) {
    return status < 400 || status > 499 || status == 400 || status == 408 ||
           status == 429;
}

std::optional<std::string> AppendUrl(std::optional<std::string> url_in,
                                     std::string const& to_append) {
    if (!url_in) {
        return std::nullopt;
    }

    if (to_append.empty()) {
        return url_in;
    }

    auto uri_components = boost::urls::parse_uri(*url_in);

    if (!uri_components) {
        return std::nullopt;
    }

    boost::urls::url url = uri_components.value();
    url.normalize();
    // The c_str here is to remove extra nulls from normalizing the path.
    // Clang calls this redundant, but it is very much required.
    std::string path =
        url.path().c_str();  // NOLINT(readability-redundant-string-cstr)

    // This sizing may not be perfect, but should be close enough on average.
    // The extra to is to account for a '/' and possible a '?'.
    path.reserve(url.path().size() + to_append.size() + url.query().length() +
                 2);

    // We want a single '/' between things.
    bool path_has_trailing_slash =
        !path.empty() && path[path.length() - 1] == '/';
    bool append_has_leading_slash = to_append[0] == '/';

    // One other the other already has a '/', so we can just append them.
    if ((path_has_trailing_slash && !append_has_leading_slash) ||
        (!path_has_trailing_slash && append_has_leading_slash)) {
        path.append(to_append);
    } else if (!path_has_trailing_slash && !append_has_leading_slash) {
        // Neither had a '/', so we need to add one.
        path.append("/");
        path.append(to_append);
    } else {
        // Both have a '/' so append the second starting after the '/'.
        path.append(to_append, 1, to_append.length() - 1);
    }

    url.set_path(path);
    url.normalize();
    return url.c_str();
}

}  // namespace launchdarkly::network
