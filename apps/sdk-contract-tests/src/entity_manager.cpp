#include "entity_manager.hpp"
#include "config/client.hpp"
#include "context_builder.hpp"

using launchdarkly::LogLevel;
using namespace launchdarkly::client_side;

EntityManager::EntityManager(boost::asio::any_io_executor executor,
                             launchdarkly::Logger& logger)
    : entities_(),
      counter_{0},
      executor_{std::move(executor)},
      logger_{logger} {}

static launchdarkly::Context MakeContext(nlohmann::json value) {
    return launchdarkly::ContextBuilder().kind("bob", "foo").build();
}

std::optional<std::string> EntityManager::create(ConfigParams in) {
    std::string id = std::to_string(counter_++);

    auto config_builder = ConfigBuilder(in.credential);

    auto default_endpoints =
        launchdarkly::client_side::Defaults::ServiceEndpoints();

    auto endpoints =
        EndpointsBuilder()
            .EventsBaseUrl(default_endpoints.EventsBaseUrl())
            .PollingBaseUrl(default_endpoints.PollingBaseUrl())
            .StreamingBaseUrl(default_endpoints.StreamingBaseUrl());

    auto datasource = DataSourceBuilder();

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

    if (in.streaming) {
        if (in.streaming->baseUri) {
            endpoints.StreamingBaseUrl(*in.streaming->baseUri);
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

    if (in.events) {
        ConfigEventParams const& events = *in.events;

        if (events.baseUri) {
            endpoints.EventsBaseUrl(*events.baseUri);
        }

        auto event_config = EventsBuilder();

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

        config_builder.Events(std::move(event_config));
    }

    config_builder.ServiceEndpoints(std::move(endpoints));

    if (in.clientSide->evaluationReasons) {
        datasource.WithReasons(*in.clientSide->evaluationReasons);
    }

    if (in.clientSide->useReport) {
        datasource.UseReport(*in.clientSide->useReport);
    }

    config_builder.DataSource(std::move(datasource));

    auto config = config_builder.Build();
    if (!config) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "entity_manager: couldn't build config: " << config.error();
        return std::nullopt;
    }

    auto client = std::make_unique<Client>(
        std::move(*config), MakeContext(in.clientSide->initialContext));

    std::chrono::milliseconds waitForClient = std::chrono::seconds(5);
    if (in.startWaitTimeMs) {
        waitForClient = std::chrono::milliseconds(*in.startWaitTimeMs);
    }
    client->WaitForReadySync(waitForClient);

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
    CommandParams params) {
    auto it = entities_.find(id);
    if (it == entities_.end()) {
        return tl::make_unexpected("entity not found");
    }
    return it->second.Command(params);
}
