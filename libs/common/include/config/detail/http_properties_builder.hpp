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

    HttpPropertiesBuilder& connect_timeout(
        std::chrono::milliseconds connect_timeout);

    HttpPropertiesBuilder& read_timeout(std::chrono::milliseconds read_timeout);

    HttpPropertiesBuilder& wrapper_name(std::string wrapper_name);

    HttpPropertiesBuilder& wrapper_version(std::string wrapper_version);

    HttpPropertiesBuilder& custom_headers(
        std::map<std::string, std::string> base_headers);

    [[nodiscard]] HttpProperties build() const;

   private:
    std::chrono::milliseconds connect_timeout_{};
    std::chrono::milliseconds read_timeout_{};
    std::string wrapper_name_;
    std::string wrapper_version_;
    std::map<std::string, std::string> base_headers_;
};

}  // namespace launchdarkly::config::detail
