#pragma once

#include <chrono>
#include <map>
#include <string>
#include <vector>

#include "config/detail/http_properties_builder.hpp"
#include "http_properties.hpp"

namespace launchdarkly::config::detail {

template <typename SDK>
class HttpPropertiesBuilder {
   public:
    HttpPropertiesBuilder() = default;

    HttpPropertiesBuilder& ConnectTimeout(
        std::chrono::milliseconds connect_timeout);

    HttpPropertiesBuilder& ReadTimeout(std::chrono::milliseconds read_timeout);

    HttpPropertiesBuilder& WrapperName(std::string wrapper_name);

    HttpPropertiesBuilder& WrapperVersion(std::string wrapper_version);

    HttpPropertiesBuilder& CustomHeaders(
        std::map<std::string, std::string> base_headers);

    [[nodiscard]] HttpProperties Build() const;

   private:
    std::chrono::milliseconds connect_timeout_{};
    std::chrono::milliseconds read_timeout_{};
    std::string wrapper_name_;
    std::string wrapper_version_;
    std::map<std::string, std::string> base_headers_;
};

}  // namespace launchdarkly::config::detail
