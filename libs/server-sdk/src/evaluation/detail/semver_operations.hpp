#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace launchdarkly::server_side::evaluation::detail {

class SemVer {
   public:
    using VersionType = unsigned long long;

    using Token = std::variant<uint64_t, std::string>;

    SemVer(VersionType major,
           VersionType minor,
           VersionType patch,
           std::optional<std::vector<Token>> prerelease);

    [[nodiscard]] SemVer::VersionType Major() const;
    [[nodiscard]] SemVer::VersionType Minor() const;
    [[nodiscard]] SemVer::VersionType Patch() const;
    [[nodiscard]] std::optional<std::vector<Token>> const& Prerelease() const;

   private:
    VersionType major_;
    VersionType minor_;
    VersionType patch_;
    std::optional<std::vector<Token>> prerelease_;
};

bool operator<(SemVer::Token const& lhs, SemVer::Token const& rhs);

bool operator==(SemVer const& lhs, SemVer const& rhs);
bool operator<(SemVer const& lhs, SemVer const& rhs);
bool operator>(SemVer const& lhs, SemVer const& rhs);
[[nodiscard]] std::optional<SemVer> ToSemVer(std::string const& value);
}  // namespace launchdarkly::server_side::evaluation::detail
