#pragma once

#include <chrono>
#include <map>
#include <string>
#include <vector>

#include "config/detail/built/http_properties.hpp"
#include "http_properties_builder.hpp"

namespace launchdarkly::config::detail::builders {

template <typename SDK>
class HttpPropertiesBuilder {
   public:
    HttpPropertiesBuilder();

    HttpPropertiesBuilder(built::HttpProperties properties);

    HttpPropertiesBuilder& ConnectTimeout(
        std::chrono::milliseconds connect_timeout);

    HttpPropertiesBuilder& ReadTimeout(std::chrono::milliseconds read_timeout);

    HttpPropertiesBuilder& ResponseTimeout(
        std::chrono::milliseconds response_timeout);

    HttpPropertiesBuilder& WrapperName(std::string wrapper_name);

    HttpPropertiesBuilder& WrapperVersion(std::string wrapper_version);

    HttpPropertiesBuilder& CustomHeaders(
        std::map<std::string, std::string> base_headers);

    [[nodiscard]] built::HttpProperties Build() const;

   private:
    std::chrono::milliseconds connect_timeout_;
    std::chrono::milliseconds read_timeout_;
    std::chrono::milliseconds response_timeout_;
    std::string wrapper_name_;
    std::string wrapper_version_;
    std::map<std::string, std::string> base_headers_;
    std::string user_agent_;
};

}  // namespace launchdarkly::config::detail::builders
