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
    // Resolve dot segments in the path. The query is not normalized, so its
    // percent-encoding stays as written.
    boost_url.normalize_path();

    host_ = uri_components->host();
    // The target is the percent-encoded path and query. The Beast backend
    // sends it as the request target without changes. The path and query are
    // joined here because encoded_target() asserts on older Boost releases
    // after the path was normalized.
    path_ = std::string(boost_url.encoded_path());
    auto const encoded_query = uri_components->encoded_query();
    if (!encoded_query.empty()) {
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
    auto segments = url.segments();
    // A trailing '/' on the URL is an empty last segment. Remove it so that a
    // single '/' separates the URL from the appended path.
    if (!segments.empty() && segments.back().empty()) {
        segments.pop_back();
    }

    // Each part between '/' characters is one segment. The URL library
    // percent-encodes the characters that are not allowed in a segment.
    std::size_t start = 0;
    while (start <= to_append.size()) {
        std::size_t end = to_append.find('/', start);
        if (end == std::string::npos) {
            end = to_append.size();
        }
        if (end > start) {
            segments.push_back(to_append.substr(start, end - start));
        }
        start = end + 1;
    }

    // Resolve dot segments such as "..".
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
    // Percent-encode every character outside the unreserved set. The URL
    // library's own parameter encoding differs between releases, so the
    // result must not depend on it.
    auto const encoded_key =
        boost::urls::encode(key, boost::urls::unreserved_chars);
    auto const encoded_value =
        boost::urls::encode(value, boost::urls::unreserved_chars);
    url.encoded_params().append({encoded_key, encoded_value});
    return std::string(url.buffer());
}

}  // namespace launchdarkly::network
