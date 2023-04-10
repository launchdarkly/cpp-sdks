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
    HttpPropertiesBuilder& connect_timeout(
        std::chrono::milliseconds connect_timeout) {
        connect_timeout_ = connect_timeout;
    }

    HttpPropertiesBuilder& read_timeout(
        std::chrono::milliseconds read_timeout) {
        read_timeout_ = read_timeout;
    }

    HttpPropertiesBuilder& wrapper_name(std::string wrapper_name) {
        wrapper_name_ = wrapper_name;
    }

    HttpPropertiesBuilder& wrapper_version(std::string wrapper_version) {
        wrapper_version_ = wrapper_version;
    }

    HttpPropertiesBuilder& custom_headers(
        std::map<std::string, std::string> base_headers) {
        base_headers_ = base_headers;
    }

    HttpProperties build() const {

        if (!wrapper_name_.empty()) {
            std::map<std::string, std::string> headers_with_wrapper(base_headers_);
            headers_with_wrapper["X-LaunchDarkly-Wrapper"] =
                wrapper_name_ + "/" + wrapper_version_;
            return {connect_timeout_, read_timeout_, "", headers_with_wrapper};
        }
        return {connect_timeout_, read_timeout_, "", base_headers_};
    }

   private:
    std::chrono::milliseconds connect_timeout_;
    std::chrono::milliseconds read_timeout_;
    std::string wrapper_name_;
    std::string wrapper_version_;
    std::map<std::string, std::string> base_headers_;
};

}  // namespace launchdarkly::config::detail
