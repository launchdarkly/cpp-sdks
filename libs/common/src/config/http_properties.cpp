#include <utility>

#include <launchdarkly/config/shared/built/http_properties.hpp>

namespace launchdarkly::config::detail::built {

HttpProperties::HttpProperties(std::chrono::milliseconds connect_timeout,
                               std::chrono::milliseconds read_timeout,
                               std::chrono::milliseconds write_timeout,
                               std::chrono::milliseconds response_timeout,
                               std::string user_agent,
                               std::map<std::string, std::string> base_headers)
    : connect_timeout_(connect_timeout),
      read_timeout_(read_timeout),
      write_timeout_(write_timeout),
      response_timeout_(response_timeout),
      user_agent_(std::move(user_agent)),
      base_headers_(std::move(base_headers)) {}

std::chrono::milliseconds HttpProperties::ConnectTimeout() const {
    return connect_timeout_;
}

std::chrono::milliseconds HttpProperties::ReadTimeout() const {
    return read_timeout_;
}

std::chrono::milliseconds HttpProperties::WriteTimeout() const {
    return write_timeout_;
}

std::chrono::milliseconds HttpProperties::ResponseTimeout() const {
    return response_timeout_;
}

std::string const& HttpProperties::UserAgent() const {
    return user_agent_;
}

std::map<std::string, std::string> const& HttpProperties::BaseHeaders() const {
    return base_headers_;
}

bool operator==(HttpProperties const& lhs, HttpProperties const& rhs) {
    return lhs.ReadTimeout() == rhs.ReadTimeout() &&
           lhs.WriteTimeout() == rhs.WriteTimeout() &&
           lhs.ConnectTimeout() == rhs.ConnectTimeout() &&
           lhs.BaseHeaders() == rhs.BaseHeaders() &&
           lhs.UserAgent() == rhs.UserAgent();
}

}  // namespace launchdarkly::config::detail::built
