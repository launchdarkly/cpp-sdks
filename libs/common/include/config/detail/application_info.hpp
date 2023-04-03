#pragma once

#include "logger.hpp"

#include <optional>
#include <string>
#include <tl/expected.hpp>
#include <vector>
#include "error.hpp"

namespace launchdarkly::config::detail {

class ApplicationInfo {
   public:
    ApplicationInfo() = default;
    ApplicationInfo& app_identifier(std::string app_id);
    ApplicationInfo& app_version(std::string version);
    [[nodiscard]] std::optional<std::string> build(Logger& logger) const;

   private:
    struct Tag {
        std::string key;
        std::string value;
        std::optional<Error> error;
        Tag(std::string key, std::string value);
        [[nodiscard]] tl::expected<std::string, Error> build() const;
    };
    std::vector<Tag> tags_;
    ApplicationInfo& add_tag(std::string key, std::string value);
};

bool ValidChar(char c);
std::optional<Error> IsValidTag(std::string const& key,
                                std::string const& value);

}  // namespace launchdarkly::config::detail
