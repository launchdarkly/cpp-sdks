#include "events/detail/summary_state.hpp"

namespace launchdarkly::events::detail {

Summarizer::Summarizer(std::chrono::system_clock::time_point start)
    : start_time_(start), features_() {}

static std::unordered_set<std::string> CopyKinds(
    std::vector<std::string_view> const& kinds) {
    std::unordered_set<std::string> kind_set;
    kind_set.insert(kinds.begin(), kinds.end());
    return kind_set;
}

void Summarizer::update(client::FeatureEventParams const& event) {
    auto const& kinds = event.context.kinds();

    auto it = features_.find(event.key);
    if (it == features_.end()) {
        // TODO(cwaldren): propagate the default value
        features_.emplace(event.key, State{Value::null(), CopyKinds(kinds)});
    } else {
        it = it->second.context_kinds_.insert(kinds.begin(), kinds.end());
    }

    if (auto reason = event.eval_result.detail().reason()) {
        if (reason->get().kind() == "ERROR" &&
            reason->get().error_kind() == "FLAG_NOT_FOUND") {
            it->second.counters_[VariationKey(std::nullopt, std::nullopt)]
                .increment();
            auto counter = it->second.counters_.find(
                VariationKey(std::nullopt, std::nullopt));
            if (counter == it->second.counters_.end()) {
                counter = it->second.counters_.emplace(
                    VariationKey(std::nullopt, std::nullopt))
            }
            it->second.counters_[VariationKey(std::nullopt, std::nullopt)] =
        }
    }
}
Summarizer::VariationKey::VariationKey(Version version,
                                       std::optional<VariationIndex> variation)
    : version(version), variation(variation) {}

Summarizer::State::State(Value defaultVal,
                         std::unordered_set<std::string> context_kinds)
    : default_(std::move(defaultVal)),
      context_kinds_(std::move(context_kinds)) {}
}  // namespace launchdarkly::events::detail
