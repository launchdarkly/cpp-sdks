#include "operators.hpp"
#include "detail/semver_operations.hpp"
#include "detail/timestamp_operations.hpp"

#include <boost/regex.hpp>

namespace launchdarkly::server_side::evaluation::operators {

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
bool SemverOp(Value const& context_value,
              Value const& clause_value,
              Callable&& op) {
    return StringOp(context_value, clause_value,
                    [op = std::move(op)](std::string const& context,
                                         std::string const& clause) {
                        auto context_semver = detail::ToSemVer(context);
                        if (!context_semver) {
                            return false;
                        }

                        auto clause_semver = detail::ToSemVer(clause);
                        if (!clause_semver) {
                            return false;
                        }

                        return op(*context_semver, *clause_semver);
                    });
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
        // std::runtime_error can be thrown when a call
        // to regex_search results in an "everlasting" search
        return false;
    }
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
