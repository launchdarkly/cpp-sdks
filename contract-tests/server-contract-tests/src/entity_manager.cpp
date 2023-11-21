#include "entity_manager.hpp"
#include <boost/json/parse.hpp>

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/serialization/json_context.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>

using launchdarkly::LogLevel;
using namespace launchdarkly::server_side;

EntityManager::EntityManager(boost::asio::any_io_executor executor,
                             launchdarkly::Logger& logger)
    : counter_{0}, executor_{std::move(executor)}, logger_{logger} {}

std::optional<std::string> EntityManager::create(ConfigParams const& in) {
    std::string id = std::to_string(counter_++);

    auto config_builder = ConfigBuilder(in.credential);

    // The contract test service sets endpoints in a way that is disallowed
    // for users. Specifically, it may set just 1 of the 3 endpoints, whereas
    // we require all 3 to be set.
    //
    // To avoid that error being detected, we must configure the Endpoints
    // builder with the 3 default URLs, which we can fetch by just calling Build
    // on a new builder. That way when the contract tests set just 1 URL,
    // the others have already been "set" so no error occurs.
    auto const default_endpoints =
        *config::builders::EndpointsBuilder().Build();

    auto& endpoints =
        config_builder.ServiceEndpoints()
            .EventsBaseUrl(default_endpoints.EventsBaseUrl())
            .PollingBaseUrl(default_endpoints.PollingBaseUrl())
            .StreamingBaseUrl(default_endpoints.StreamingBaseUrl());

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
    auto datasystem = config::builders::DataSystemBuilder::BackgroundSync();

    if (in.streaming) {
        if (in.streaming->baseUri) {
            endpoints.StreamingBaseUrl(*in.streaming->baseUri);
        }
        if (in.streaming->initialRetryDelayMs) {
            auto streaming = decltype(datasystem)::Streaming();
            streaming.InitialReconnectDelay(
                std::chrono::milliseconds(*in.streaming->initialRetryDelayMs));
            datasystem.Synchronizer(std::move(streaming));
        }
    }

    if (in.polling) {
        if (in.polling->baseUri) {
            endpoints.PollingBaseUrl(*in.polling->baseUri);
        }
        if (!in.streaming) {
            auto method = decltype(datasystem)::Polling();
            if (in.polling->pollIntervalMs) {
                method.PollInterval(
                    std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::milliseconds(
                            *in.polling->pollIntervalMs)));
            }
            datasystem.Synchronizer(std::move(method));
        }
    }

    config_builder.DataSystem().Method(std::move(datasystem));

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

    if (in.tags) {
        if (in.tags->applicationId) {
            config_builder.AppInfo().Identifier(*in.tags->applicationId);
        }
        if (in.tags->applicationVersion) {
            config_builder.AppInfo().Version(*in.tags->applicationVersion);
        }
    }

    auto config = config_builder.Build();
    if (!config) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "entity_manager: couldn't build config: " << config.error();
        return std::nullopt;
    }

    auto client = std::make_unique<Client>(std::move(*config));

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
