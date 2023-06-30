#include "operators.hpp"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <regex>
namespace launchdarkly::server_side::evaluation::operators {

using boost_date = boost::gregorian::date;

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

std::optional<boost_date> ToDate(Value const& value) {
    if (value.Type() == Value::Type::kNumber) {
        return std::nullopt;
        // return std::chrono::system_clock::from_time_t(value.AsInt());
    } else if (value.Type() == Value::Type::kString) {
        try {
            auto x = boost::gregorian::date_from_iso_string(value.AsString());
            boost_date date(x);
            return date;
        } catch (std::exception e) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

template <typename Callable>
bool TimeOp(Value const& context_value,
            Value const& clause_value,
            Callable&& op) {
    auto context_date = ToDate(context_value);
    if (!context_date) {
        return false;
    }
    auto clause_date = ToDate(clause_value);
    if (!clause_date) {
        return false;
    }
    return op(*context_date, *clause_date);
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

bool RegexMatch(std::string const& context_value,
                std::string const& clause_value) {
    return std::regex_match(context_value, std::regex(clause_value));
}

bool Before(boost_date const& context_value, boost_date const& clause_value) {
    return context_value < clause_value;
}

bool After(boost_date const& context_value, boost_date const& clause_value) {
    return context_value > clause_value;
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
            return TimeOp(context_value, clause_value, Before);
        case data_model::Clause::Op::kAfter:
            return TimeOp(context_value, clause_value, After);
        case data_model::Clause::Op::kSemVerEqual:
            return false; /* unimplemented */
        case data_model::Clause::Op::kSemVerLessThan:
            return false; /* unimplemented */
        case data_model::Clause::Op::kSemVerGreaterThan:
            return false; /* unimplemented */
        default:
            return false;
    }
}

}  // namespace launchdarkly::server_side::evaluation::operators
