#include "entity_manager.hpp"

#include <launchdarkly/config/client.hpp>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/serialization/json_context.hpp>

#include <boost/json.hpp>

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
            config_builder.HttpProperties().Proxy(
                HttpPropertiesBuilder::ProxyBuilder().HttpProxy(
                    *in.proxy->httpProxy));
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
