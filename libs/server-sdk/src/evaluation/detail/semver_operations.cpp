#include "semver_operations.hpp"

#include <launchdarkly/detail/c_binding_helpers.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include <iterator>

namespace launchdarkly::server_side::evaluation::detail {

/*
 * Official SemVer 2.0 Regex
 * https://semver.org/#is-there-a-suggested-regular-expression-regex-to-check-a-semver-string
 *
 * Modified for LaunchDarkly usage to allow missing minor and patch versions,
 * i.e. "1" means "1.0.0" or "1.2" means "1.2.0".
 */
char const* const kSemVerRegex =
    R"(^(?<major>0|[1-9]\d*)(\.(?<minor>0|[1-9]\d*))?(\.(?<patch>0|[1-9]\d*))?(?:-(?<prerelease>(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\.(?:0|[1-9]\d*|\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?(?:\+([0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?$)";

/**
 * Cache the parsed regex so it doesn't need to be rebuilt constantly.
 *
 * From Boost docs:
 * Class basic_regex and its typedefs regex and wregex are thread safe, in that
 * compiled regular expressions can safely be shared between threads.
 */
static boost::regex const& SemVerRegex() {
    static boost::regex regex{kSemVerRegex, boost::regex_constants::no_except};
    LD_ASSERT(regex.status() == 0);
    return regex;
}

SemVer::SemVer(VersionType major,
               VersionType minor,
               VersionType patch,
               std::vector<Token> prerelease)
    : major_(major), minor_(minor), patch_(patch), prerelease_(prerelease) {}

SemVer::SemVer(VersionType major, VersionType minor, VersionType patch)
    : major_(major), minor_(minor), patch_(patch), prerelease_(std::nullopt) {}

SemVer::SemVer() : SemVer(0, 0, 0) {}

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
    // At this point, lhs and rhs have equal major/minor/patch versions.
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

std::optional<SemVer> SemVer::Parse(std::string const& value) {
    if (value.empty()) {
        return std::nullopt;
    }

    boost::regex const& semver_regex = SemVerRegex();
    boost::smatch match;

    try {
        if (!boost::regex_match(value, match, semver_regex)) {
            // Not a semantic version.
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

    try {
        if (match["major"].matched) {
            major = boost::lexical_cast<SemVer::VersionType>(match["major"]);
        }
        if (match["minor"].matched) {
            minor = boost::lexical_cast<SemVer::VersionType>(match["minor"]);
        }
        if (match["patch"].matched) {
            patch = boost::lexical_cast<SemVer::VersionType>(match["patch"]);
        }
    } catch (boost::bad_lexical_cast) {
        return std::nullopt;
    }

    if (!match["prerelease"].matched) {
        return SemVer{major, minor, patch};
    }

    std::vector<std::string> tokens;
    boost::split(tokens, match["prerelease"], boost::is_any_of("."));

    std::vector<SemVer::Token> prerelease;

    std::transform(
        tokens.begin(), tokens.end(), std::back_inserter(prerelease),
        [](std::string const& token) -> SemVer::Token {
            try {
                return boost::lexical_cast<SemVer::VersionType>(token);
            } catch (boost::bad_lexical_cast) {
                return token;
            }
        });

    return SemVer{major, minor, patch, prerelease};
}

std::ostream& operator<<(std::ostream& out, SemVer::Token const& sv) {
    if (sv.index() == 0) {
        out << std::get<0>(sv);
    } else {
        out << std::get<1>(sv);
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, SemVer const& sv) {
    out << sv.Major() << "." << sv.Minor() << "." << sv.Patch();
    if (sv.Prerelease()) {
        out << "-";

        for (auto it = sv.Prerelease()->begin(); it != sv.Prerelease()->end();
             ++it) {
            out << *it;
            if (std::next(it) != sv.Prerelease()->end()) {
                out << ".";
            }
        }
    }
    return out;
}
}  // namespace launchdarkly::server_side::evaluation::detail
