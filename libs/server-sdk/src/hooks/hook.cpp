#include <launchdarkly/server_side/hooks/hook.hpp>

#include <utility>

namespace launchdarkly::server_side::hooks {

// HookContext implementation

HookContext& HookContext::Set(std::string key,
                               std::shared_ptr<std::any> value) {
    data_[std::move(key)] = std::move(value);
    return *this;
}

std::optional<std::shared_ptr<std::any>> HookContext::Get(
    std::string const& key) const {
    if (const auto it = data_.find(key); it != data_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool HookContext::Has(std::string const& key) const {
    return data_.find(key) != data_.end();
}

// HookMetadata implementation

HookMetadata::HookMetadata(std::string name) : name_(std::move(name)) {}

std::string_view HookMetadata::Name() const {
    return name_;
}

// EvaluationSeriesData implementation

EvaluationSeriesData::EvaluationSeriesData() = default;

EvaluationSeriesData::EvaluationSeriesData(
    std::map<std::string, DataEntry> data)
    : data_(std::move(data)) {}

std::optional<Value> EvaluationSeriesData::Get(std::string const& key) const {
    auto it = data_.find(key);
    if (it != data_.end() && it->second.value) {
        return it->second.value;
    }
    return std::nullopt;
}

std::optional<std::shared_ptr<std::any>> EvaluationSeriesData::GetShared(
    std::string const& key) const {
    if (const auto it = data_.find(key); it != data_.end() && it->second.shared) {
        return it->second.shared;
    }
    return std::nullopt;
}

bool EvaluationSeriesData::Has(std::string const& key) const {
    return data_.find(key) != data_.end();
}

std::vector<std::string> EvaluationSeriesData::Keys() const {
    std::vector<std::string> keys;
    keys.reserve(data_.size());
    for (auto const& [key, _] : data_) {
        keys.push_back(key);
    }
    return keys;
}

// EvaluationSeriesDataBuilder implementation

EvaluationSeriesDataBuilder::EvaluationSeriesDataBuilder() = default;

EvaluationSeriesDataBuilder::EvaluationSeriesDataBuilder(
    EvaluationSeriesData const& data)
    : data_(data.data_) {}

EvaluationSeriesDataBuilder& EvaluationSeriesDataBuilder::Set(std::string key,
                                                               Value value) {
    EvaluationSeriesData::DataEntry entry;
    entry.value = std::move(value);
    data_[std::move(key)] = std::move(entry);
    return *this;
}

EvaluationSeriesDataBuilder& EvaluationSeriesDataBuilder::SetShared(
    std::string key,
    std::shared_ptr<std::any> value) {
    EvaluationSeriesData::DataEntry entry;
    entry.shared = std::move(value);
    data_[std::move(key)] = std::move(entry);
    return *this;
}

EvaluationSeriesData EvaluationSeriesDataBuilder::Build() const {
    return EvaluationSeriesData(data_);
}

// EvaluationSeriesContext implementation

EvaluationSeriesContext::EvaluationSeriesContext(
    std::string flag_key,
    Context const& context,
    Value default_value,
    std::string method,
    HookContext const& hook_context,
    std::optional<std::string> environment_id)
    : flag_key_(std::move(flag_key)),
      context_(context),
      default_value_(std::move(default_value)),
      method_(std::move(method)),
      hook_context_(hook_context),
      environment_id_(std::move(environment_id)) {}

std::string_view EvaluationSeriesContext::FlagKey() const {
    return flag_key_;
}

Context const& EvaluationSeriesContext::EvaluationContext() const {
    return context_;
}

Value EvaluationSeriesContext::DefaultValue() const {
    return default_value_;
}

std::string_view EvaluationSeriesContext::Method() const {
    return method_;
}

std::optional<std::string_view> EvaluationSeriesContext::EnvironmentId() const {
    if (environment_id_) {
        return *environment_id_;
    }
    return std::nullopt;
}

HookContext const& EvaluationSeriesContext::HookCtx() const {
    return hook_context_;
}

// TrackSeriesContext implementation

TrackSeriesContext::TrackSeriesContext(
    Context const& context,
    std::string key,
    std::optional<double> metric_value,
    std::optional<Value> data,
    HookContext const& hook_context,
    std::optional<std::string> environment_id)
    : context_(context),
      key_(std::move(key)),
      metric_value_(metric_value),
      data_(std::move(data)),
      hook_context_(hook_context),
      environment_id_(std::move(environment_id)) {}

Context const& TrackSeriesContext::TrackContext() const {
    return context_;
}

std::string_view TrackSeriesContext::Key() const {
    return key_;
}

std::optional<double> TrackSeriesContext::MetricValue() const {
    return metric_value_;
}

std::optional<Value> TrackSeriesContext::Data() const {
    return data_;
}

std::optional<std::string_view> TrackSeriesContext::EnvironmentId() const {
    if (environment_id_) {
        return *environment_id_;
    }
    return std::nullopt;
}

HookContext const& TrackSeriesContext::HookCtx() const {
    return hook_context_;
}

// Hook base implementation

EvaluationSeriesData Hook::BeforeEvaluation(
    EvaluationSeriesContext const& series_context,
    EvaluationSeriesData data) {
    return data;
}

EvaluationSeriesData Hook::AfterEvaluation(
    EvaluationSeriesContext const& series_context,
    EvaluationSeriesData data,
    EvaluationDetail<Value> const& detail) {
    return data;
}

void Hook::AfterTrack(TrackSeriesContext const& series_context) {}

}  // namespace launchdarkly::server_side::hooks
