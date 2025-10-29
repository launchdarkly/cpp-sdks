/**
 * @file tracing_hook.cpp
 * @brief Implementation of OpenTelemetry tracing hook for LaunchDarkly
 */

#include <launchdarkly/server_side/integrations/otel/tracing_hook.hpp>

#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_detail.hpp>
#include <launchdarkly/data/evaluation_reason.hpp>
#include <launchdarkly/value.hpp>
#include <launchdarkly/detail/serialization/json_value.hpp>

#include <opentelemetry/context/context.h>
#include <opentelemetry/trace/context.h>
#include <opentelemetry/trace/span_context.h>

#include <boost/json.hpp>

#include <string>

namespace launchdarkly::server_side::integrations::otel {
// OpenTelemetry semantic convention attribute names
namespace otel_attrs {
constexpr auto FEATURE_FLAG_KEY = "feature_flag.key";
constexpr auto FEATURE_FLAG_PROVIDER_NAME = "feature_flag.provider.name";
constexpr auto FEATURE_FLAG_CONTEXT_ID = "feature_flag.context.id";
constexpr auto FEATURE_FLAG_CONTEXT_KEY = "feature_flag.context.key";
constexpr auto FEATURE_FLAG_SET_ID = "feature_flag.set.id";
constexpr auto FEATURE_FLAG_RESULT_VALUE = "feature_flag.result.value";
constexpr auto FEATURE_FLAG_RESULT_VARIATION_INDEX =
    "feature_flag.result.variationIndex";
constexpr auto FEATURE_FLAG_RESULT_REASON_IN_EXPERIMENT =
    "feature_flag.result.reason.inExperiment";

constexpr auto PROVIDER_NAME = "LaunchDarkly";
constexpr auto EVENT_NAME = "feature_flag";
} // namespace otel_attrs

// Keys for series data
namespace series_keys {
constexpr auto SPAN = "otel.span";
}

// Keys for hook context
namespace hook_ctx_keys {
constexpr auto SPAN = "otel.span";
}

TracingHook::TracingHook()
    : TracingHook(TracingHookOptionsBuilder().Build()) {
}

TracingHook::TracingHook(TracingHookOptions options)
    : options_(std::move(options)),
      metadata_("LaunchDarkly OpenTelemetry Tracing Hook") {
    // Options are validated by the builder
}

hooks::HookMetadata const& TracingHook::Metadata() const {
    return metadata_;
}

opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer>
TracingHook::GetTracer() {
    const auto provider = opentelemetry::trace::Provider::GetTracerProvider();
    return provider->GetTracer("launchdarkly-cpp-server", "1.0.0");
}

opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>
TracingHook::GetActiveSpan(hooks::HookContext const& hook_ctx) {
    // First, check if a span was provided via HookContext
    if (const auto maybe_span_any = hook_ctx.Get(hook_ctx_keys::SPAN);
        maybe_span_any.has_value()) {
        try {
            auto& span_any = *maybe_span_any.value();
            const auto span_ptr = std::any_cast<
                opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>>(
                &span_any);
            if (span_ptr && *span_ptr) {
                return *span_ptr;
            }
        } catch (const std::bad_any_cast&) {
            // Ignore and fall through to get active span from context
        }
    }

    // Fall back to getting active span from OpenTelemetry context
    return opentelemetry::trace::Tracer::GetCurrentSpan();
}

std::optional<std::string> TracingHook::GetEnvironmentId(
    hooks::EvaluationSeriesContext const& series_context) const {
    // Configured environment ID takes precedence
    if (options_.EnvironmentId().has_value()) {
        return options_.EnvironmentId();
    }

    // Fall back to environment ID from context (available after init)
    if (const auto env_id = series_context.EnvironmentId(); env_id.
        has_value()) {
        return std::string(env_id.value());
    }

    return std::nullopt;
}

hooks::EvaluationSeriesData TracingHook::BeforeEvaluation(
    hooks::EvaluationSeriesContext const& series_context,
    hooks::EvaluationSeriesData data) {
    // Only create spans if configured to do so
    if (!options_.CreateSpans()) {
        return data;
    }

    // Any exception will be handled by the SDK and it will log that an error
    // has happened in hook execution.
    const auto tracer = GetTracer();

    // Build span name: "LDClient.{method}"
    std::string span_name = "LDClient.";
    span_name.append(series_context.Method());

    // Create span options
    opentelemetry::trace::StartSpanOptions options;
    options.kind = opentelemetry::trace::SpanKind::kInternal;

    if (auto span = tracer->
        StartSpan(span_name, options)) {
        // Add attributes to span
        span->SetAttribute(otel_attrs::FEATURE_FLAG_KEY,
                           std::string(series_context.FlagKey()));
        span->SetAttribute(
            otel_attrs::FEATURE_FLAG_CONTEXT_KEY,
            std::string(series_context.EvaluationContext().CanonicalKey()));

        // Store span in series data for AfterEvaluation
        auto builder = hooks::EvaluationSeriesDataBuilder(data);
        builder.SetShared(series_keys::SPAN,
                          std::make_shared<std::any>(span));
        return builder.Build();
    }

    return data;
}

void TracingHook::AddFeatureFlagEvent(
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> const& span,
    hooks::EvaluationSeriesContext const& series_context,
    EvaluationDetail<Value> const& detail) const {
    // Copy all dynamic data to owned storage
    auto flag_key = std::string(series_context.FlagKey());
    auto context_id =
        std::string(series_context.EvaluationContext().CanonicalKey());
    auto provider_name = std::string(otel_attrs::PROVIDER_NAME);

    std::string env_id_str;
    bool has_env_id = false;
    if (auto env_id = GetEnvironmentId(series_context); env_id.has_value()) {
        env_id_str = env_id.value();
        has_env_id = true;
    }

    std::string value_str;
    bool has_value = false;
    if (options_.IncludeValue()) {
        boost::json::value json_value;
        boost::json::value_from(detail.Value(), json_value);
        value_str = boost::json::serialize(json_value);
        has_value = true;
    }

    int64_t variation_index = 0;
    bool has_variation_index = detail.VariationIndex().has_value();
    if (has_variation_index) {
        variation_index = static_cast<int64_t>(detail.VariationIndex().value());
    }

    bool in_experiment = false;
    if (detail.Reason().has_value() && detail.Reason()->InExperiment()) {
        in_experiment = true;
    }

    std::vector<std::pair<opentelemetry::nostd::string_view,
                          opentelemetry::common::AttributeValue>>
        attributes;

    attributes.emplace_back(otel_attrs::FEATURE_FLAG_KEY, flag_key);
    attributes.emplace_back(otel_attrs::FEATURE_FLAG_PROVIDER_NAME,
                            provider_name);
    attributes.emplace_back(otel_attrs::FEATURE_FLAG_CONTEXT_ID, context_id);

    if (has_env_id) {
        attributes.emplace_back(otel_attrs::FEATURE_FLAG_SET_ID, env_id_str);
    }
    if (has_value) {
        attributes.emplace_back(otel_attrs::FEATURE_FLAG_RESULT_VALUE,
                                value_str);
    }
    if (in_experiment) {
        attributes.emplace_back(
            otel_attrs::FEATURE_FLAG_RESULT_REASON_IN_EXPERIMENT,
            in_experiment);
    }
    if (has_variation_index) {
        attributes.emplace_back(
            otel_attrs::FEATURE_FLAG_RESULT_VARIATION_INDEX, variation_index);
    }

    span->AddEvent(otel_attrs::EVENT_NAME, attributes);
}

hooks::EvaluationSeriesData TracingHook::AfterEvaluation(
    hooks::EvaluationSeriesContext const& series_context,
    hooks::EvaluationSeriesData data,
    EvaluationDetail<Value> const& detail) {
    // First, end any span we created in BeforeEvaluation
    if (options_.CreateSpans()) {
        if (const auto maybe_span_any = data.GetShared(series_keys::SPAN);
            maybe_span_any.has_value()) {
            const auto& span_any = *maybe_span_any.value();
            const auto span_ptr =
                std::any_cast<opentelemetry::nostd::shared_ptr<
                    opentelemetry::trace::Span>>(&span_any);
            if (span_ptr && *span_ptr) {
                (*span_ptr)->End();
            }
        }
    }

    // Get the active span (either from hook context or global context)

    // Only add event if there's an active span
    if (const auto active_span = GetActiveSpan(series_context.HookCtx());
        active_span && active_span->GetContext().IsValid()) {
        AddFeatureFlagEvent(active_span, series_context, detail);
    }
    return data;
}
} // namespace launchdarkly::server_side::integrations::otel