#pragma once

#include "../data_interfaces/store/istore.hpp"
#include "evaluation_error.hpp"
#include "evaluation_stack.hpp"

#include <launchdarkly/context.hpp>
#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/segment.hpp>

#include <tl/expected.hpp>

#include <string>

namespace launchdarkly::server_side::evaluation {

[[nodiscard]] tl::expected<bool, Error> Match(
    data_model::Flag::Rule const&,
    Context const&,
    data_interfaces::IStore const& store,
    EvaluationStack& stack);

[[nodiscard]] tl::expected<bool, Error> Match(data_model::Clause const&,
                                              Context const&,
                                              data_interfaces::IStore const&,
                                              EvaluationStack&);

[[nodiscard]] tl::expected<bool, Error> Match(
    data_model::Segment::Rule const& rule,
    Context const& context,
    data_interfaces::IStore const& store,
    EvaluationStack& stack,
    std::string const& key,
    std::string const& salt);

[[nodiscard]] tl::expected<bool, Error> MatchSegment(
    data_model::Clause const&,
    Context const&,
    data_interfaces::IStore const&,
    EvaluationStack& stack);

[[nodiscard]] tl::expected<bool, Error> MatchNonSegment(
    data_model::Clause const&,
    Context const&);

[[nodiscard]] tl::expected<bool, Error> Contains(
    data_model::Segment const&,
    Context const&,
    data_interfaces::IStore const& store,
    EvaluationStack& stack);

[[nodiscard]] bool MaybeNegate(data_model::Clause const& clause, bool value);

[[nodiscard]] bool IsTargeted(Context const&,
                              std::vector<std::string> const&,
                              std::vector<data_model::Segment::Target> const&);

[[nodiscard]] bool IsUser(Context const& context);

}  // namespace launchdarkly::server_side::evaluation
