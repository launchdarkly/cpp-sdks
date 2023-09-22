#pragma once

#include <launchdarkly/context.hpp>
#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/segment.hpp>

#include "../data_retrieval/interfaces/data_store/data_store.hpp"
#include "detail/evaluation_stack.hpp"
#include "evaluation_error.hpp"

#include <tl/expected.hpp>

#include <string>

namespace launchdarkly::server_side::evaluation {

[[nodiscard]] tl::expected<bool, Error> Match(
    data_model::Flag::Rule const&,
    Context const&,
    data_retrieval::IDataStore const& store,
    detail::EvaluationStack& stack);

[[nodiscard]] tl::expected<bool, Error> Match(data_model::Clause const&,
                                              Context const&,
                                              data_retrieval::IDataStore const&,
                                              detail::EvaluationStack&);

[[nodiscard]] tl::expected<bool, Error> Match(
    data_model::Segment::Rule const& rule,
    Context const& context,
    data_retrieval::IDataStore const& store,
    detail::EvaluationStack& stack,
    std::string const& key,
    std::string const& salt);

[[nodiscard]] tl::expected<bool, Error> MatchSegment(
    data_model::Clause const&,
    Context const&,
    data_retrieval::IDataStore const&,
    detail::EvaluationStack& stack);

[[nodiscard]] tl::expected<bool, Error> MatchNonSegment(
    data_model::Clause const&,
    Context const&);

[[nodiscard]] tl::expected<bool, Error> Contains(
    data_model::Segment const&,
    Context const&,
    data_retrieval::IDataStore const& store,
    detail::EvaluationStack& stack);

[[nodiscard]] bool MaybeNegate(data_model::Clause const& clause, bool value);

[[nodiscard]] bool IsTargeted(Context const&,
                              std::vector<std::string> const&,
                              std::vector<data_model::Segment::Target> const&);

[[nodiscard]] bool IsUser(Context const& context);

}  // namespace launchdarkly::server_side::evaluation
