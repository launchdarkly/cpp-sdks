#include "mode_sources.hpp"

#include "cache_initializer.hpp"
#include "source_factories.hpp"

#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/serialization/json_context.hpp>

#include <boost/json.hpp>

#include <utility>
#include <variant>

namespace launchdarkly::client_side::data_sources {

namespace {

// Lets std::visit dispatch to a different lambda per variant alternative.
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

FDv2RequestConfig MakeRequestConfig(std::string base_url,
                                    ModeSourceParams const& params,
                                    std::string serialized_context,
                                    bool use_post) {
    return FDv2RequestConfig{std::move(base_url), params.http_properties,
                             std::move(serialized_context),
                             use_post ? FDv2ContextTransport::kPostBody
                                      : FDv2ContextTransport::kGetPath,
                             params.with_reasons};
}

// Maps a wall-clock instant onto the monotonic clock the poll interval is timed
// against. steady_clock is used while running (for monotonicity) but cannot
// persist across a restart, so the last poll is stored as system_clock and
// mapped back here.
std::chrono::steady_clock::time_point ToSteadyClock(
    std::chrono::system_clock::time_point instant) {
    auto const now = std::chrono::system_clock::now();
    return std::chrono::steady_clock::now() - (now - instant);
}

// The FDv1 fallback reuses the client's own FDv1 polling source, which is
// already pointed at the client SDK's FDv1 endpoints.
std::unique_ptr<IFDv2SynchronizerFactory> MakeFDv1Fallback(
    FDv2Config::FDv1FallbackConfig const& fallback,
    ModeSourceParams const& params) {
    auto const defaults =
        config::shared::Defaults<config::shared::ClientSDK>::PollingConfig();

    config::shared::built::DataSourceConfig<config::shared::ClientSDK> const
        fdv1_config{
            config::shared::built::PollingConfig<config::shared::ClientSDK>{
                fallback.poll_interval, defaults.polling_get_path,
                defaults.polling_report_path, defaults.min_polling_interval},
            params.with_reasons,
            // FDv2 supersedes the REPORT transport, so the option is
            // ignored when both are configured.
            /* use_report= */ false};

    auto endpoints =
        fallback.base_url_override
            ? config::shared::built::
                  ServiceEndpoints{*fallback.base_url_override,
                                   params.endpoints.StreamingBaseUrl(),
                                   params.endpoints.EventsBaseUrl()}
            : params.endpoints;

    return std::make_unique<FDv1PollingAdapterFactory>(
        params.executor, params.logger, std::move(endpoints), fdv1_config,
        params.http_properties, params.context);
}

}  // namespace

ModeSources BuildModeSources(FDv2Config const& config,
                             ConnectionMode mode,
                             ModeSourceParams const& params) {
    ModeSources sources;

    auto const definition = config.modes.find(mode);
    if (definition == config.modes.end()) {
        LD_LOG(params.logger, LogLevel::kError)
            << "fdv2: connection mode "
            << config::shared::GetConnectionModeName(mode)
            << " is not configured";
        return sources;
    }

    auto const serialized_context =
        boost::json::serialize(boost::json::value_from(params.context));

    for (auto const& entry : definition->second.initializers) {
        std::visit(
            overloaded{
                [&](FDv2Config::CacheConfig const&) {
                    sources.initializers.push_back(
                        std::make_unique<FDv2CacheInitializerFactory>(
                            params.cache, params.context, params.logger));
                },
                [&](FDv2Config::PollingConfig const& polling) {
                    sources.initializers.push_back(
                        std::make_unique<FDv2PollingInitializerFactory>(
                            params.executor, params.logger,
                            MakeRequestConfig(
                                polling.base_url_override.value_or(
                                    params.polling_base_url),
                                params, serialized_context, config.use_post)));
                },
            },
            entry);
    }

    // The interval a poll is rate limited against is measured from the last
    // time this context was polled, which survives restarts so that repeated
    // launches cannot produce a burst of requests.
    std::optional<std::chrono::steady_clock::time_point> last_poll;
    if (auto const freshness = params.cache->ReadFreshness(params.context)) {
        last_poll = ToSteadyClock(*freshness);
    }

    for (auto const& entry : definition->second.synchronizers) {
        std::visit(
            overloaded{
                [&](FDv2Config::PollingConfig const& polling) {
                    sources.synchronizers.push_back(
                        std::make_unique<FDv2PollingSynchronizerFactory>(
                            params.executor, params.logger,
                            MakeRequestConfig(
                                polling.base_url_override.value_or(
                                    params.polling_base_url),
                                params, serialized_context, config.use_post),
                            polling.poll_interval, last_poll));
                },
                [&](FDv2Config::StreamingConfig const& streaming) {
                    sources.synchronizers.push_back(
                        std::make_unique<FDv2StreamingSynchronizerFactory>(
                            params.executor, params.logger,
                            MakeRequestConfig(
                                streaming.base_url_override.value_or(
                                    params.streaming_base_url),
                                params, serialized_context, config.use_post),
                            MakeRequestConfig(params.polling_base_url, params,
                                              serialized_context,
                                              config.use_post),
                            streaming.initial_reconnect_delay));
                },
            },
            entry);
    }

    if (auto const& fallback = definition->second.fdv1_fallback) {
        sources.synchronizers.push_back(MakeFDv1Fallback(*fallback, params));
    }

    return sources;
}

}  // namespace launchdarkly::client_side::data_sources
