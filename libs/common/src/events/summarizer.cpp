#include "events/detail/summarizer.hpp"

namespace launchdarkly::events::detail {

Summarizer::Summarizer(std::chrono::system_clock::time_point start)
    : start_time_(start) {}

bool Summarizer::Empty() const {
    return features_.empty();
}

std::unordered_map<Summarizer::FlagKey, Summarizer::State> const&
Summarizer::features() const {
    return features_;
}

static bool FlagNotFound(client::FeatureEventParams const& event) {
    if (auto reason = event.eval_result.detail().reason()) {
        return reason->get().kind() == "ERROR" &&
               reason->get().error_kind() == "FLAG_NOT_FOUND";
    }
    return false;
}

void Summarizer::Update(client::FeatureEventParams const& event) {
    auto const& kinds = event.context.kinds();

    auto feature_state_iterator =
        features_.try_emplace(event.key, event.default_).first;

    feature_state_iterator->second.context_kinds.insert(kinds.begin(),
                                                        kinds.end());

    decltype(std::begin(
        feature_state_iterator->second.counters)) summary_counter;

    if (FlagNotFound(event)) {
        summary_counter =
            feature_state_iterator->second.counters
                .try_emplace(VariationKey(),
                             feature_state_iterator->second.default_)
                .first;

    } else {
        auto key = VariationKey(event.eval_result.version(),
                                event.eval_result.detail().variation_index());
        summary_counter =
            feature_state_iterator->second.counters
                .try_emplace(key, event.eval_result.detail().value())
                .first;
    }

    summary_counter->second.Increment();
}

Summarizer& Summarizer::Finish(Time end_time) {
    end_time_ = end_time;
    return *this;
}

Summarizer::Time Summarizer::start_time() const {
    return start_time_;
}

Summarizer::Time Summarizer::end_time() const {
    return end_time_;
}

Summarizer::VariationKey::VariationKey(Version version,
                                       std::optional<VariationIndex> variation)
    : version(version), variation(variation) {}

Summarizer::VariationKey::VariationKey()
    : version(std::nullopt), variation(std::nullopt) {}

Summarizer::VariationSummary::VariationSummary(Value value)
    : count_(0), value_(std::move(value)) {}

void Summarizer::VariationSummary::Increment() {
    count_++;
}

Value const& Summarizer::VariationSummary::value() const {
    return value_;
}

std::int32_t Summarizer::VariationSummary::count() const {
    return count_;
}

Summarizer::State::State(Value default_value)
    : default_(std::move(default_value)) {}
}  // namespace launchdarkly::events::detail
