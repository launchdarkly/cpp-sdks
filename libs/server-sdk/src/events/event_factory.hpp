#pragma once

#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_detail.hpp>
#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/events/data/events.hpp>

#include <optional>
#include <string>

namespace launchdarkly::server_side {

class EventFactory {
    enum class ReasonPolicy {
        Default = 0,
        Send = 1,
    };

   public:
    [[nodiscard]] static EventFactory WithReasons();
    [[nodiscard]] static EventFactory WithoutReasons();

    [[nodiscard]] events::InputEvent UnknownFlag(std::string const& key,
                                                 Context const& ctx,
                                                 EvaluationDetail<Value> detail,
                                                 Value default_val) const;

    [[nodiscard]] events::InputEvent Eval(
        std::string const& key,
        Context const& ctx,
        std::optional<data_model::Flag> const& flag,
        EvaluationDetail<Value> detail,
        Value default_value,
        std::optional<std::string> prereq_of) const;

    [[nodiscard]] events::InputEvent Identify(Context ctx) const;

    [[nodiscard]] events::InputEvent Custom(
        Context const& ctx,
        std::string event_name,
        std::optional<Value> data,
        std::optional<double> metric_value) const;

   private:
    EventFactory(ReasonPolicy reason_policy);
    [[nodiscard]] events::InputEvent FeatureRequest(
        std::string const& key,
        Context const& ctx,
        std::optional<data_model::Flag> const& flag,
        EvaluationDetail<Value> detail,
        Value default_val,
        std::optional<std::string> prereq_of) const;

    ReasonPolicy const reason_policy_;
    std::function<events::Date()> now_;
};
}  // namespace launchdarkly::server_side
