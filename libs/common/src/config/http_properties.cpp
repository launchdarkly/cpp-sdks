#include "config/detail/http_properties.hpp"

namespace launchdarkly::config::detail {

HttpProperties::HttpProperties(std::chrono::milliseconds connect_timeout,
                               std::chrono::milliseconds read_timeout,
                               std::string user_agent,
                               std::map<std::string, std::string> base_headers)
    : connect_timeout_(connect_timeout),
      read_timeout_(read_timeout),
      user_agent_(user_agent),
      base_headers_(base_headers) {}

std::chrono::milliseconds HttpProperties::connect_timeout() const {
    return connect_timeout_;
}

std::chrono::milliseconds HttpProperties::read_timeout() const {
    return read_timeout_;
}

std::string const& HttpProperties::user_agent() const {
    return user_agent_;
}

std::map<std::string, std::string> const& HttpProperties::base_headers() const {
    return base_headers_;
}

}  // namespace launchdarkly::config::detail
