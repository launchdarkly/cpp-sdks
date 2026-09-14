#include "mode_sources.hpp"

#include "cache_initializer.hpp"
#include "source_factories.hpp"

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

// The poll interval is measured on the monotonic clock, but the last poll was
// recorded on the wall clock so that it could be persisted. An instant in the
// future means the wall clock moved backwards since it was written, which
// says nothing usable about when the last poll happened.
std::optional<std::chrono::steady_clock::time_point> ToSteadyClock(
    std::optional<std::chrono::system_clock::time_point> instant) {
    if (!instant) {
        return std::nullopt;
    }
    auto const now = std::chrono::system_clock::now();
    if (*instant > now) {
        return std::nullopt;
    }
    return std::chrono::steady_clock::now() - (now - *instant);
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

    auto polling_config = [&](FDv2Config::PollingConfig const& polling) {
        return MakeRequestConfig(
            polling.base_url_override.value_or(params.polling_base_url), params,
            serialized_context, config.use_post);
    };
    auto streaming_config = [&](FDv2Config::StreamingConfig const& streaming) {
        return MakeRequestConfig(
            streaming.base_url_override.value_or(params.streaming_base_url),
            params, serialized_context, config.use_post);
    };

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
                            polling_config(polling)));
                },
            },
            entry);
    }

    // The interval a poll is rate limited against is measured from the last
    // time this context was polled, which survives restarts so that repeated
    // launches cannot produce a burst of requests.
    auto const last_poll =
        ToSteadyClock(params.cache->ReadFreshness(params.context));

    for (auto const& entry : definition->second.synchronizers) {
        std::visit(
            overloaded{
                [&](FDv2Config::PollingConfig const& polling) {
                    sources.synchronizers.push_back(
                        std::make_unique<FDv2PollingSynchronizerFactory>(
                            params.executor, params.logger,
                            polling_config(polling), polling.poll_interval,
                            last_poll));
                },
                [&](FDv2Config::StreamingConfig const& streaming) {
                    sources.synchronizers.push_back(
                        std::make_unique<FDv2StreamingSynchronizerFactory>(
                            params.executor, params.logger,
                            streaming_config(streaming),
                            polling_config(FDv2Config::PollingConfig{
                                std::chrono::seconds::zero(), std::nullopt}),
                            streaming.initial_reconnect_delay));
                },
            },
            entry);
    }

    return sources;
}

}  // namespace launchdarkly::client_side::data_sources
