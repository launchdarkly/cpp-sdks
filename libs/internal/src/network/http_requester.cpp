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
    : error_message_(std::move(error_message)), error_(true), status_(0) {}

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

HttpRequest::HttpRequest(std::string const& url,
                         HttpMethod method,
                         config::shared::built::HttpProperties properties,
                         HttpRequest::BodyType body)
    : properties_(std::move(properties)),
      method_(method),
      body_(std::move(body)),
      url_(url),
      port_(std::nullopt) {
    auto uri_components = boost::urls::parse_uri(url);

    // If the URI cannot be parsed, then the request is not valid.
    if (!uri_components) {
        valid_ = false;
        return;
    }

    boost::urls::url boost_url = uri_components.value();
    // Resolve dot segments and make slashes consistent. Only the path is
    // normalized: normalizing the query would decode escapes such as %26,
    // changing which characters act as separators.
    boost_url.normalize_path();

    host_ = uri_components->host();
    // Keep the percent-encoding. The Beast backend sends this string as the
    // request target verbatim, so a decoded space, '#', '&' or CR LF coming
    // from a server-supplied value (for example the FDv2 "basis" selector
    // state) would corrupt the request line.
    path_ = std::string(boost_url.encoded_path());
    auto const encoded_query = uri_components->encoded_query();
    if (!encoded_query.empty()) {
        // For a boost beast request we need the query string in the path.
        path_ += "?";
        path_ += std::string(encoded_query);
    }

    is_https_ = uri_components->scheme_id() == boost::urls::scheme::https;
    if (uri_components->has_port()) {
        port_ = uri_components->port();
    }
    valid_ = true;
}

HttpRequest::HttpRequest(HttpRequest& base_request,
                         config::shared::built::HttpProperties properties)
    : properties_(std::move(properties)),
      host_(base_request.host_),
      port_(base_request.port_),
      path_(base_request.path_),
      is_https_(base_request.is_https_),
      valid_(base_request.valid_),
      url_(base_request.url_),
      method_(base_request.method_),
      body_(std::move(base_request.body_)) {}

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
    // Normalize only the path (dot segments, slashes). The query is left as
    // written so its percent-encoding survives.
    url.normalize_path();
    std::string path(url.encoded_path());

    // This sizing may not be perfect, but should be close enough on average.
    // The extra to is to account for a '/' and possible a '?'.
    path.reserve(url.encoded_path().size() + to_append.size() +
                 url.encoded_query().size() + 2);

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

    // The appended path may itself be percent-encoded (a base64url context,
    // a redirect Location). Keep those escapes, encode anything else that is
    // not allowed in a path, and reject malformed escapes.
    auto const encoded_path = boost::urls::make_pct_string_view(path);
    if (!encoded_path) {
        return std::nullopt;
    }
    url.set_encoded_path(*encoded_path);
    url.normalize_path();
    return std::string(url.buffer());
}

std::optional<std::string> AppendQueryParam(std::optional<std::string> url_in,
                                            std::string const& key,
                                            std::string const& value) {
    if (!url_in) {
        return std::nullopt;
    }

    auto uri_components = boost::urls::parse_uri(*url_in);
    if (!uri_components) {
        return std::nullopt;
    }

    boost::urls::url url = uri_components.value();
    url.params().append({key, value});
    return std::string(url.buffer());
}

}  // namespace launchdarkly::network
