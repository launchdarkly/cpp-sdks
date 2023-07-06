#include "operators.hpp"
#include "detail/timestamp_operations.hpp"

#include <launchdarkly/detail/c_binding_helpers.hpp>

#include <date/date.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <iostream>
#include <sstream>
namespace launchdarkly::server_side::evaluation::operators {

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

template <typename Callable>
bool StringOp(Value const& context_value,
              Value const& clause_value,
              Callable&& op) {
    if (!(context_value.Type() == Value::Type::kString &&
          clause_value.Type() == Value::Type::kString)) {
        return false;
    }
    return op(context_value.AsString(), clause_value.AsString());
}

template <typename Callable>
bool TimeOp(Value const& context_value,
            Value const& clause_value,
            Callable&& op) {
    auto context_tp = detail::ToTimepoint(context_value);
    if (!context_tp) {
        return false;
    }
    auto clause_tp = detail::ToTimepoint(clause_value);
    if (!clause_tp) {
        return false;
    }
    return op(*context_tp, *clause_tp);
}

bool StartsWith(std::string const& context_value,
                std::string const& clause_value) {
    return clause_value.size() <= context_value.size() &&
           std::equal(clause_value.begin(), clause_value.end(),
                      context_value.begin());
}

bool EndsWith(std::string const& context_value,
              std::string const& clause_value) {
    return clause_value.size() <= context_value.size() &&
           std::equal(clause_value.rbegin(), clause_value.rend(),
                      context_value.rbegin());
}

bool Contains(std::string const& context_value,
              std::string const& clause_value) {
    return context_value.find(clause_value) != std::string::npos;
}

/* RegexMatch uses boost::regex instead of std::regex, because the former
 * appears to be significantly more performant according to boost benchmarks.
 * For more information, see here:
 * https://www.boost.org/doc/libs/1_82_0/libs/regex/doc/html/boost_regex/background/performance.html
 */
bool RegexMatch(std::string const& context_value,
                std::string const& clause_value) {
    /* See here for FAQ on boost::regex exceptions:
     * https://
     * www.boost.org/doc/libs/1_82_0/libs/regex/doc/html/boost_regex/background/faq.html
     * */
    try {
        return boost::regex_search(context_value, boost::regex(clause_value));
    } catch (boost::bad_expression) {
        // boost::bad_expression can be thrown by basic_regex when compiling a
        // regular expression.
        return false;
    } catch (boost::regex_error) {
        // boost::regex_error thrown on stack exhaustion
        return false;
    } catch (std::runtime_error) {
        // std::runtime_error can be thrown when a call to basic_regex::imbue
        // tries to open a message catalogue that doesn't exist, or when a call
        // to regex_search or regex_match results in an "everlasting" search, or
        // when a call to RegEx::GrepFiles or RegEx::FindFiles tries to open a
        // file that cannot be opened
        return false;
    }
}

class SemVer {
   public:
    using VersionType = unsigned long long;

    using Token = std::variant<uint64_t, std::string>;

    SemVer(VersionType major,
           VersionType minor,
           VersionType patch,
           std::optional<std::vector<Token>> prerelease)
        : major_(major),
          minor_(minor),
          patch_(patch),
          prerelease_(prerelease) {}

    SemVer::VersionType Major() const { return major_; }
    SemVer::VersionType Minor() const { return minor_; }
    SemVer::VersionType Patch() const { return patch_; }
    std::optional<std::vector<Token>> const& Prerelease() const {
        return prerelease_;
    }

   private:
    VersionType major_;
    VersionType minor_;
    VersionType patch_;
    std::optional<std::vector<Token>> prerelease_;
};

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

template <typename Callable>
bool SemverOp(Value const& context_value,
              Value const& clause_value,
              Callable&& op) {
    return StringOp(context_value, clause_value,
                    [op = std::move(op)](std::string const& context,
                                         std::string const& clause) {
                        auto context_semver = ToSemVer(context);
                        if (!context_semver) {
                            return false;
                        }

                        auto clause_semver = ToSemVer(clause);
                        if (!clause_semver) {
                            return false;
                        }

                        return op(*context_semver, *clause_semver);
                    });
}

bool Match(data_model::Clause::Op op,
           Value const& context_value,
           Value const& clause_value) {
    switch (op) {
        case data_model::Clause::Op::kIn:
            return context_value == clause_value;
        case data_model::Clause::Op::kStartsWith:
            return StringOp(context_value, clause_value, StartsWith);
        case data_model::Clause::Op::kEndsWith:
            return StringOp(context_value, clause_value, EndsWith);
        case data_model::Clause::Op::kMatches:
            return StringOp(context_value, clause_value, RegexMatch);
        case data_model::Clause::Op::kContains:
            return StringOp(context_value, clause_value, Contains);
        case data_model::Clause::Op::kLessThan:
            return context_value < clause_value;
        case data_model::Clause::Op::kLessThanOrEqual:
            return context_value <= clause_value;
        case data_model::Clause::Op::kGreaterThan:
            return context_value > clause_value;
        case data_model::Clause::Op::kGreaterThanOrEqual:
            return context_value >= clause_value;
        case data_model::Clause::Op::kBefore:
            return TimeOp(
                context_value, clause_value,
                [](auto const& lhs, auto const& rhs) { return lhs < rhs; });
        case data_model::Clause::Op::kAfter:
            return TimeOp(
                context_value, clause_value,
                [](auto const& lhs, auto const& rhs) { return lhs > rhs; });
        case data_model::Clause::Op::kSemVerEqual:
            return SemverOp(
                context_value, clause_value,
                [](auto const& lhs, auto const& rhs) { return lhs == rhs; });
        case data_model::Clause::Op::kSemVerLessThan:
            return SemverOp(
                context_value, clause_value,
                [](auto const& lhs, auto const& rhs) { return lhs < rhs; });
        case data_model::Clause::Op::kSemVerGreaterThan:
            return SemverOp(
                context_value, clause_value,
                [](auto const& lhs, auto const& rhs) { return lhs > rhs; });
        default:
            return false;
    }
}

}  // namespace launchdarkly::server_side::evaluation::operators
