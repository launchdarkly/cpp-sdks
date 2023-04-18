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

    [[nodiscard]] std::chrono::milliseconds ConnectTimeout() const;
    [[nodiscard]] std::chrono::milliseconds ReadTimeout() const;
    [[nodiscard]] std::string const& UserAgent() const;
    [[nodiscard]] std::map<std::string, std::string> const& BaseHeaders() const;

   private:
    std::chrono::milliseconds connect_timeout_;
    std::chrono::milliseconds read_timeout_;
    std::string user_agent_;
    std::map<std::string, std::string> base_headers_;

    // TODO: Proxy.
};

bool operator==(HttpProperties const& lhs, HttpProperties const& rhs);

}  // namespace launchdarkly::config::detail
