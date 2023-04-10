#pragma once

#include <chrono>
#include <map>
#include <string>
#include <vector>

namespace launchdarkly::config::detail {

class HttpProperties final {
   public:
    HttpProperties(std::chrono::milliseconds connect_timeout,
                   std::chrono::milliseconds read_timeout,
                   std::string user_agent,
                   std::map<std::string, std::string> base_headers);

    std::chrono::milliseconds connect_timeout() const;
    std::chrono::milliseconds read_timeout() const;
    std::string const& user_agent() const;
    std::map<std::string, std::string> const& base_headers() const;

   private:
    std::chrono::milliseconds connect_timeout_;
    std::chrono::milliseconds read_timeout_;
    std::string user_agent_;
    std::map<std::string, std::string> base_headers_;

    // TODO: Proxy.
};

}  // namespace launchdarkly::config::detail
