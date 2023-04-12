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

static bool FlagNotFound(client::FeatureEventParams const& event) {
    if (auto reason = event.eval_result.detail().reason()) {
        return reason->get().kind() == "ERROR" &&
               reason->get().error_kind() == "FLAG_NOT_FOUND";
    }
    return false;
}

void Summarizer::update(client::FeatureEventParams const& event) {
    auto const& kinds = event.context.kinds();

    // TODO(cwaldren): Value::null() should be replaced with the default value
    // from the evaluation.

    auto default_value = Value::null();
    auto feature_state_iterator =
        features_.try_emplace(event.key, std::move(default_value)).first;

    feature_state_iterator->second.context_kinds_.insert(kinds.begin(),
                                                         kinds.end());

    decltype(std::begin(feature_state_iterator->second.counters_)) summary_counter;

    if (FlagNotFound(event)) {
        auto key = VariationKey();
        summary_counter = feature_state_iterator->second.counters_
                      .try_emplace(std::move(key),
                                   feature_state_iterator->second.default_)
                      .first;

    } else {
        auto key = VariationKey(event.eval_result.version(),
                                event.eval_result.detail().variation_index());
        summary_counter =
            feature_state_iterator->second.counters_
                .try_emplace(std::move(key), event.eval_result.detail().value())
                .first;
    }

    summary_counter->second.Increment();
}
Summarizer::VariationKey::VariationKey(Version version,
                                       std::optional<VariationIndex> variation)
    : version(version), variation(variation) {}

Summarizer::VariationKey::VariationKey()
    : version(std::nullopt), variation(std::nullopt) {}

Summarizer::VariationKey::VariationKey(Summarizer::VariationKey const& key) {}

Summarizer::VariationSummary::VariationSummary(Value value)
    : count(0), value(std::move(value)) {}

void Summarizer::VariationSummary::Increment() {
    // todo(cwaldren): prevent overflow?
    count++;
}

Summarizer::State::State(Value defaultVal)
    : default_(std::move(defaultVal)), context_kinds_() {}
}  // namespace launchdarkly::events::detail
