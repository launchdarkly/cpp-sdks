#include "entity_manager.hpp"

#include <launchdarkly/config/client.hpp>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/serialization/json_context.hpp>

#include <boost/json.hpp>

#include <chrono>
#include <optional>
#include <utility>

using launchdarkly::LogLevel;
using namespace launchdarkly::client_side;

EntityManager::EntityManager(boost::asio::any_io_executor executor,
                             launchdarkly::Logger& logger)
    : entities_(),
      counter_{0},
      executor_{std::move(executor)},
      logger_{logger} {}

static tl::expected<launchdarkly::Context, launchdarkly::JsonError>
ParseContext(nlohmann::json value) {
    auto boost_json_val = boost::json::parse(value.dump());
    return boost::json::value_to<
        tl::expected<launchdarkly::Context, launchdarkly::JsonError>>(
        boost_json_val);
}

static std::chrono::seconds ToSeconds(uint64_t milliseconds) {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::milliseconds(milliseconds));
}

// The harness names modes the way the configuration API does. Desktop
// provides three of them. The mobile- and browser-only modes are not
// configurable here.
static std::optional<ConnectionMode> ParseConnectionMode(
    std::string const& name) {
    if (name == "streaming") {
        return ConnectionMode::kStreaming;
    }
    if (name == "polling") {
        return ConnectionMode::kPolling;
    }
    if (name == "offline") {
        return ConnectionMode::kOffline;
    }
    return std::nullopt;
}

// Builds exactly the pipeline the harness asked for. No cache initializer is
// added, so the requests the SDK makes are the ones the harness expects.
static FDv2Builder::Mode BuildMode(
    ConfigModeDefinitionParams const& definition,
    std::optional<ConfigPollingParams> const& fdv1_fallback) {
    FDv2Builder::Mode mode;

    if (definition.initializers) {
        for (auto const& initializer : *definition.initializers) {
            if (!initializer.polling) {
                continue;
            }
            auto polling = FDv2Builder::Polling();
            if (initializer.polling->baseUri) {
                polling.BaseUrl(*initializer.polling->baseUri);
            }
            if (initializer.polling->pollIntervalMs) {
                polling.PollInterval(
                    ToSeconds(*initializer.polling->pollIntervalMs));
            }
            mode.Initializer(std::move(polling));
        }
    }

    if (definition.synchronizers) {
        for (auto const& synchronizer : *definition.synchronizers) {
            if (synchronizer.streaming) {
                auto streaming = FDv2Builder::Streaming();
                if (synchronizer.streaming->baseUri) {
                    streaming.BaseUrl(*synchronizer.streaming->baseUri);
                }
                if (synchronizer.streaming->initialRetryDelayMs) {
                    streaming.InitialReconnectDelay(std::chrono::milliseconds(
                        *synchronizer.streaming->initialRetryDelayMs));
                }
                mode.Synchronizer(std::move(streaming));
            } else if (synchronizer.polling) {
                auto polling = FDv2Builder::Polling();
                if (synchronizer.polling->baseUri) {
                    polling.BaseUrl(*synchronizer.polling->baseUri);
                }
                if (synchronizer.polling->pollIntervalMs) {
                    polling.PollInterval(
                        ToSeconds(*synchronizer.polling->pollIntervalMs));
                }
                mode.Synchronizer(std::move(polling));
            }
        }
    }

    if (fdv1_fallback) {
        auto fallback = FDv2Builder::FDv1Fallback();
        if (fdv1_fallback->baseUri) {
            fallback.BaseUrl(*fdv1_fallback->baseUri);
        }
        if (fdv1_fallback->pollIntervalMs) {
            fallback.PollInterval(ToSeconds(*fdv1_fallback->pollIntervalMs));
        }
        mode.FallbackToFDv1(std::move(fallback));
    } else {
        mode.DisableFDv1Fallback();
    }

    return mode;
}

static bool HasPipelines(ConfigDataSystemParams const& cfg) {
    return (cfg.initializers && !cfg.initializers->empty()) ||
           (cfg.synchronizers && !cfg.synchronizers->empty());
}

static FDv2Builder BuildFDv2(ConfigDataSystemParams const& cfg) {
    FDv2Builder fdv2;

    if (cfg.useDefaultDataSystem.value_or(false)) {
        return fdv2;
    }

    if (cfg.connectionModeConfig) {
        auto const& modes = *cfg.connectionModeConfig;
        if (modes.initialConnectionMode) {
            if (auto const mode =
                    ParseConnectionMode(*modes.initialConnectionMode)) {
                fdv2.InitialMode(*mode);
            }
        }
        if (modes.customConnectionModes) {
            for (auto const& [name, definition] :
                 *modes.customConnectionModes) {
                if (auto const mode = ParseConnectionMode(name)) {
                    fdv2.CustomizeMode(*mode,
                                       BuildMode(definition, cfg.fdv1Fallback));
                }
            }
        }
        return fdv2;
    }

    // A pipeline given without a mode wrapper describes the mode the SDK
    // starts in.
    if (HasPipelines(cfg)) {
        ConfigModeDefinitionParams top_level;
        top_level.initializers = cfg.initializers;
        top_level.synchronizers = cfg.synchronizers;
        fdv2.InitialMode(ConnectionMode::kStreaming);
        fdv2.CustomizeMode(ConnectionMode::kStreaming,
                           BuildMode(top_level, cfg.fdv1Fallback));
    }

    return fdv2;
}

std::optional<std::string> EntityManager::create(ConfigParams const& in) {
    std::string id = std::to_string(counter_++);

    auto config_builder = ConfigBuilder(in.credential);

    auto default_endpoints =
        launchdarkly::client_side::Defaults::ServiceEndpoints();

    auto& endpoints =
        config_builder.ServiceEndpoints()
            .EventsBaseUrl(default_endpoints.EventsBaseUrl())
            .PollingBaseUrl(default_endpoints.PollingBaseUrl())
            .StreamingBaseUrl(default_endpoints.StreamingBaseUrl());

    if (in.proxy) {
        if (in.proxy->httpProxy) {
            config_builder.HttpProperties().Proxy(*in.proxy->httpProxy);
        }
    }

    if (in.serviceEndpoints) {
        if (in.serviceEndpoints->streaming) {
            endpoints.StreamingBaseUrl(*in.serviceEndpoints->streaming);
        }
        if (in.serviceEndpoints->polling) {
            endpoints.PollingBaseUrl(*in.serviceEndpoints->polling);
        }
        if (in.serviceEndpoints->events) {
            endpoints.EventsBaseUrl(*in.serviceEndpoints->events);
        }
    }
    auto& datasource = config_builder.DataSource();

    if (in.streaming) {
        if (in.streaming->baseUri) {
            endpoints.StreamingBaseUrl(*in.streaming->baseUri);
        }
        if (in.streaming->initialRetryDelayMs) {
            auto streaming = DataSourceBuilder::Streaming();
            streaming.InitialReconnectDelay(
                std::chrono::milliseconds(*in.streaming->initialRetryDelayMs));
            datasource.Method(std::move(streaming));
        }
    }

    if (in.polling) {
        if (in.polling->baseUri) {
            endpoints.PollingBaseUrl(*in.polling->baseUri);
        }
        if (!in.streaming) {
            auto method = DataSourceBuilder::Polling();
            if (in.polling->pollIntervalMs) {
                method.PollInterval(
                    std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::milliseconds(
                            *in.polling->pollIntervalMs)));
            }
            datasource.Method(std::move(method));
        }
    }

    auto& event_config = config_builder.Events();

    if (in.events) {
        ConfigEventParams const& events = *in.events;

        if (events.baseUri) {
            endpoints.EventsBaseUrl(*events.baseUri);
        }

        if (events.allAttributesPrivate) {
            event_config.AllAttributesPrivate(*events.allAttributesPrivate);
        }

        if (!events.globalPrivateAttributes.empty()) {
            launchdarkly::AttributeReference::SetType attrs(
                events.globalPrivateAttributes.begin(),
                events.globalPrivateAttributes.end());
            event_config.PrivateAttributes(std::move(attrs));
        }

        if (events.capacity) {
            event_config.Capacity(*events.capacity);
        }

        if (events.flushIntervalMs) {
            event_config.FlushInterval(
                std::chrono::milliseconds(*events.flushIntervalMs));
        }

    } else {
        event_config.Disable();
    }

    if (in.clientSide->evaluationReasons) {
        datasource.WithReasons(*in.clientSide->evaluationReasons);
    }

    if (in.clientSide->useReport) {
        datasource.UseReport(*in.clientSide->useReport);
    }

    // A data system configuration selects FDv2, which supersedes the
    // streaming and polling methods above.
    if (in.dataSystem) {
        datasource.Method(BuildFDv2(*in.dataSystem));
    }

    if (in.tags) {
        if (in.tags->applicationId) {
            config_builder.AppInfo().Identifier(*in.tags->applicationId);
        }
        if (in.tags->applicationVersion) {
            config_builder.AppInfo().Version(*in.tags->applicationVersion);
        }
    }

    if (in.tls) {
        auto builder = TlsBuilder();
        if (in.tls->skipVerifyPeer) {
            builder.SkipVerifyPeer(*in.tls->skipVerifyPeer);
        }
        if (in.tls->customCAFile) {
            builder.CustomCAFile(*in.tls->customCAFile);
        }
        config_builder.HttpProperties().Tls(std::move(builder));
    }

    if (in.wrapper) {
        if (!in.wrapper->name.empty()) {
            config_builder.HttpProperties().WrapperName(in.wrapper->name);
        }
        if (!in.wrapper->version.empty()) {
            config_builder.HttpProperties().WrapperVersion(in.wrapper->version);
        }
    }

    auto config = config_builder.Build();
    if (!config) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "entity_manager: couldn't build config: " << config.error();
        return std::nullopt;
    }

    auto maybe_context = ParseContext(in.clientSide->initialContext);
    if (!maybe_context) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "entity_manager: initial context provided was invalid";
        return std::nullopt;
    }

    auto client = std::make_unique<Client>(std::move(*config), *maybe_context);

    std::chrono::milliseconds waitForClient = std::chrono::seconds(5);
    if (in.startWaitTimeMs) {
        waitForClient = std::chrono::milliseconds(*in.startWaitTimeMs);
    }

    auto init = client->StartAsync();
    init.wait_for(waitForClient);

    entities_.try_emplace(id, std::move(client));

    return id;
}

bool EntityManager::destroy(std::string const& id) {
    auto it = entities_.find(id);
    if (it == entities_.end()) {
        return false;
    }

    entities_.erase(it);
    return true;
}

tl::expected<nlohmann::json, std::string> EntityManager::command(
    std::string const& id,
    CommandParams const& params) {
    auto it = entities_.find(id);
    if (it == entities_.end()) {
        return tl::make_unexpected("entity not found");
    }
    return it->second.Command(params);
}
