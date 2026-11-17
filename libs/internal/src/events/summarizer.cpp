#include <launchdarkly/events/detail/summarizer.hpp>

namespace launchdarkly::events::detail {

bool Summarizer::Empty() const {
    return features_.empty();
}

std::unordered_map<Summarizer::FlagKey, Summarizer::State> const&
Summarizer::Features() const {
    return features_;
}

void Summarizer::Update(events::FeatureEventParams const& event) {
    auto const time = event.creation_date.t;
    if (features_.empty() || time < start_time_) {
        start_time_ = time;
    }
    if (features_.empty() || time > end_time_) {
        end_time_ = time;
    }

    auto const& kinds = event.context.Kinds();

    auto feature_state_iterator =
        features_.try_emplace(event.key, event.default_).first;

    feature_state_iterator->second.context_kinds.insert(kinds.begin(),
                                                        kinds.end());

    auto summary_counter =
        feature_state_iterator->second.counters
            .try_emplace(VariationKey(event.version, event.variation),
                         event.value)
            .first;

    summary_counter->second.Increment();
}

Summarizer::Time Summarizer::StartTime() const {
    return start_time_;
}

Summarizer::Time Summarizer::EndTime() const {
    return end_time_;
}

Summarizer::VariationKey::VariationKey(std::optional<Version> version,
                                       std::optional<VariationIndex> variation)
    : version(version), variation(variation) {}

Summarizer::VariationKey::VariationKey()
    : version(std::nullopt), variation(std::nullopt) {}

Summarizer::VariationSummary::VariationSummary(::launchdarkly::Value value)
    : count_(0), value_(std::move(value)) {}

void Summarizer::VariationSummary::Increment() {
    count_++;
}

Value const& Summarizer::VariationSummary::Value() const {
    return value_;
}

std::int32_t Summarizer::VariationSummary::Count() const {
    return count_;
}

Summarizer::State::State(Value default_value)
    : default_(std::move(default_value)) {}
}  // namespace launchdarkly::events::detail
