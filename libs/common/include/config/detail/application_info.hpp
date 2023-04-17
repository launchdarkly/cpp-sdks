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
    ApplicationInfo& AppIdentifier(std::string app_id);
    ApplicationInfo& AppVersion(std::string version);
    [[nodiscard]] std::optional<std::string> Build(Logger& logger) const;

   private:
    struct Tag {
        std::string key;
        std::string value;
        std::optional<Error> error;
        Tag(std::string key, std::string value);
        [[nodiscard]] tl::expected<std::string, Error> Build() const;
    };
    std::vector<Tag> tags_;
    ApplicationInfo& AddTag(std::string key, std::string value);
};

bool ValidChar(char c);
std::optional<Error> IsValidTag(std::string const& key,
                                std::string const& value);

}  // namespace launchdarkly::config::detail
