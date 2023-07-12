#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace launchdarkly::server_side::evaluation::detail {

/**
 * Represents a LaunchDarkly-flavored Semantic Version v2.
 *
 * Semantic versions can be compared using ==, <, and >.
 *
 * The main difference from the official spec is that missing minor and patch
 * versions are allowed, i.e. "1" means "1.0.0" and "1.2" means "1.2.0".
 */
class SemVer {
   public:
    using VersionType = std::uint64_t;

    using Token = std::variant<VersionType, std::string>;

    /**
     * Constructs a SemVer representing "0.0.0".
     */
    SemVer();

    /**
     * Constructs a SemVer from a major, minor, patch, and prerelease. The
     * prerelease consists of list of string/nonzero-number tokens, e.g.
     * ["alpha", 1"].
     * @param major Major version.
     * @param minor Minor version.
     * @param patch Patch version.
     * @param prerelease Prerelease tokens.
     */
    SemVer(VersionType major,
           VersionType minor,
           VersionType patch,
           std::vector<Token> prerelease);

    /**
     * Constructs a SemVer from a major, minor, and patch.
     * @param major Major version.
     * @param minor Minor version.
     * @param patch Patch version.
     */
    SemVer(VersionType major, VersionType minor, VersionType patch);

    [[nodiscard]] SemVer::VersionType Major() const;
    [[nodiscard]] SemVer::VersionType Minor() const;
    [[nodiscard]] SemVer::VersionType Patch() const;

    [[nodiscard]] std::optional<std::vector<Token>> const& Prerelease() const;

    /**
     * Attempts to parse a semantic version string, returning std::nullopt on
     * failure. Build information is discarded.
     * @param value Version string, e.g. "1.2.3-alpha.1".
     * @return SemVer on success, or std::nullopt on failure.
     */
    [[nodiscard]] static std::optional<SemVer> Parse(std::string const& value);

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

/** Prints a SemVer to an ostream. If the SemVer was parsed from a string
 * containing a build string, it will not be present as this information
 * is discarded when parsing.
 */
std::ostream& operator<<(std::ostream& out, SemVer const& sv);

std::ostream& operator<<(std::ostream& out, SemVer::Token const& sv);

}  // namespace launchdarkly::server_side::evaluation::detail
