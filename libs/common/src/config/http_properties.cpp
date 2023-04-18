#include <utility>

#include "config/detail/http_properties.hpp"

namespace launchdarkly::config::detail {

HttpProperties::HttpProperties(std::chrono::milliseconds connect_timeout,
                               std::chrono::milliseconds read_timeout,
                               std::string user_agent,
                               std::map<std::string, std::string> base_headers)
    : connect_timeout_(connect_timeout),
      read_timeout_(read_timeout),
      user_agent_(std::move(user_agent)),
      base_headers_(std::move(base_headers)) {}

std::chrono::milliseconds HttpProperties::ConnectTimeout() const {
    return connect_timeout_;
}

std::chrono::milliseconds HttpProperties::ReadTimeout() const {
    return read_timeout_;
}

std::string const& HttpProperties::UserAgent() const {
    return user_agent_;
}

std::map<std::string, std::string> const& HttpProperties::BaseHeaders() const {
    return base_headers_;
}

bool operator==(HttpProperties const& lhs, HttpProperties const& rhs) {
    return lhs.ReadTimeout() == rhs.ReadTimeout() &&
           lhs.ConnectTimeout() == rhs.ConnectTimeout() &&
           lhs.BaseHeaders() == rhs.BaseHeaders() &&
           lhs.UserAgent() == rhs.UserAgent();
}

}  // namespace launchdarkly::config::detail
