#pragma once

#include <chrono>
#include <map>
#include <string>
#include <vector>

namespace launchdarkly::config::shared::built {

class HttpProperties final {
   public:
    HttpProperties(std::chrono::milliseconds connect_timeout,
                   std::chrono::milliseconds read_timeout,
                   std::chrono::milliseconds write_timeout,
                   std::chrono::milliseconds response_timeout,
                   std::map<std::string, std::string> base_headers);

    [[nodiscard]] std::chrono::milliseconds ConnectTimeout() const;
    [[nodiscard]] std::chrono::milliseconds ReadTimeout() const;
    [[nodiscard]] std::chrono::milliseconds WriteTimeout() const;

    [[nodiscard]] std::chrono::milliseconds ResponseTimeout() const;
    [[nodiscard]] std::map<std::string, std::string> const& BaseHeaders() const;

   private:
    std::chrono::milliseconds connect_timeout_;
    std::chrono::milliseconds read_timeout_;
    std::chrono::milliseconds write_timeout_;
    std::chrono::milliseconds response_timeout_;
    std::map<std::string, std::string> base_headers_;

    // TODO: Proxy.
};

bool operator==(HttpProperties const& lhs, HttpProperties const& rhs);

}  // namespace launchdarkly::config::shared::built
