#include "semver_operations.hpp"
#include <launchdarkly/detail/c_binding_helpers.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

namespace launchdarkly::server_side::evaluation::detail {

/*
 * Official SemVer 2.0 Regex
 * https://semver.org/#is-there-a-suggested-regular-expression-regex-to-check-a-semver-string
 */
char const* const kSemVerRegex =
    R"(^(?<major>0|[1-9]\d*)(\.(?<minor>0|[1-9]\d*))?(\.(?<patch>0|[1-9]\d*))?(?:-(?<prerelease>(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?(?:\+([0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?$)";

static boost::regex const& SemVerRegex() {
    static boost::regex regex{kSemVerRegex};
    LD_ASSERT(regex.status() == 0);
    return regex;
}

SemVer::SemVer(VersionType major,
               VersionType minor,
               VersionType patch,
               std::optional<std::vector<Token>> prerelease)
    : major_(major), minor_(minor), patch_(patch), prerelease_(prerelease) {}

SemVer::VersionType SemVer::Major() const {
    return major_;
}
SemVer::VersionType SemVer::Minor() const {
    return minor_;
}
SemVer::VersionType SemVer::Patch() const {
    return patch_;
}
std::optional<std::vector<SemVer::Token>> const& SemVer::Prerelease() const {
    return prerelease_;
}

bool operator<(SemVer::Token const& lhs, SemVer::Token const& rhs) {
    if (lhs.index() != rhs.index()) {
        /* Numeric identifiers (index 0 of variant) always have lower precedence
than non-numeric identifiers. */
        return lhs.index() < rhs.index();
    }
    if (lhs.index() == 0) {
        return std::get<0>(lhs) < std::get<0>(rhs);
    }
    return std::get<1>(lhs) < std::get<1>(rhs);
}

bool operator==(SemVer const& lhs, SemVer const& rhs) {
    return lhs.Major() == rhs.Major() && lhs.Minor() == rhs.Minor() &&
           lhs.Patch() == rhs.Patch() && lhs.Prerelease() == rhs.Prerelease();
}

bool operator<(SemVer const& lhs, SemVer const& rhs) {
    if (lhs.Major() < rhs.Major()) {
        return true;
    }
    if (lhs.Major() > rhs.Major()) {
        return false;
    }
    if (lhs.Minor() < rhs.Minor()) {
        return true;
    }
    if (lhs.Minor() > rhs.Minor()) {
        return false;
    }
    if (lhs.Patch() < rhs.Patch()) {
        return true;
    }
    if (lhs.Patch() > rhs.Patch()) {
        return false;
    }
    if (!lhs.Prerelease() && !rhs.Prerelease()) {
        return false;
    }
    if (lhs.Prerelease() && !rhs.Prerelease()) {
        return true;
    }
    if (!lhs.Prerelease() && rhs.Prerelease()) {
        return false;
    }
    return *lhs.Prerelease() < *rhs.Prerelease();
}

bool operator>(SemVer const& lhs, SemVer const& rhs) {
    return rhs < lhs;
}

std::optional<SemVer> ToSemVer(std::string const& value) {
    if (value.empty()) {
        return std::nullopt;
    }
    boost::regex const& semver_regex = SemVerRegex();
    boost::smatch match;
    try {
        if (!boost::regex_match(value, match, semver_regex)) {
            return std::nullopt;
        }
    } catch (std::runtime_error) {
        /* std::runtime_error if the complexity of matching the expression
         * against an N character string begins to exceed O(N2), or if the
         * program runs out of stack space while matching the expression
         * (if Boost.Regex is configured in recursive mode), or if the matcher
         * exhausts its permitted memory allocation (if Boost.Regex
         * is configured in non-recursive mode).*/
        return std::nullopt;
    }

    SemVer::VersionType major = 0;
    SemVer::VersionType minor = 0;
    SemVer::VersionType patch = 0;

    std::optional<std::vector<SemVer::Token>> prerelease;

    try {
        if (match["major"].matched) {
            major = std::stoull(match["major"]);
        }
        if (match["minor"].matched) {
            minor = std::stoull(match["minor"]);
        }
        if (match["patch"].matched) {
            patch = std::stoull(match["patch"]);
        }

        if (match["prerelease"].matched) {
            std::vector<std::string> tokens;
            boost::split(tokens, match["prerelease"], boost::is_any_of("."));
            if (!tokens.empty()) {
                prerelease.emplace();
                std::transform(tokens.begin(), tokens.end(),
                               std::back_inserter(*prerelease),
                               [](std::string const& token)
                                   -> std::variant<uint64_t, std::string> {
                                   try {
                                       return std::stoull(token);
                                   } catch (std::invalid_argument) {
                                       return token;
                                   }
                               });
            }
        }
    } catch (std::invalid_argument) {
        // Conversion failed.
        return std::nullopt;
    } catch (std::out_of_range) {
        // Cannot represent the value as an unsigned long long,
        return std::nullopt;
    }
    return SemVer{major, minor, patch, prerelease};
}

}  // namespace launchdarkly::server_side::evaluation::detail
