#pragma once

#include <optional>
#include <string>
#include <vector>
namespace launchdarkly::config::detail {

class ApplicationInfo {
   public:
    ApplicationInfo() = default;
    ApplicationInfo& app_identifier(std::string app_id);
    ApplicationInfo& app_version(std::string version);
    [[nodiscard]] std::optional<std::string> build() const;

   private:
    struct Tag {
        std::string key;
        std::string value;
        Tag(std::string key, std::string value);
        [[nodiscard]] std::string build() const;
    };
    std::vector<Tag> tags_;
    ApplicationInfo& add_tag(std::string key, std::string value);
};

bool valid_char(char c);
bool is_valid_tag(std::string const& key, std::string const& value);

}  // namespace launchdarkly::config::detail
