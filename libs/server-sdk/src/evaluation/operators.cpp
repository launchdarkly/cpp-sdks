#include "operators.hpp"

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
            return false;
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
            return false; /* unimplemented */
        case data_model::Clause::Op::kAfter:
            return false; /* unimplemented */
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
