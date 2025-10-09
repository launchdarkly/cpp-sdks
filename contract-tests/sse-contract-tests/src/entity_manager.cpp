#include "entity_manager.hpp"
#include "event_outbox.hpp"

using launchdarkly::LogLevel;

EntityManager::EntityManager(boost::asio::any_io_executor executor,
                             launchdarkly::Logger& logger,
                             bool use_curl)
    : counter_{0}, executor_{std::move(executor)}, logger_{logger}, use_curl_{use_curl} {}

std::optional<std::string> EntityManager::create(ConfigParams const& params) {
    std::string id = std::to_string(counter_++);

    auto poster = std::make_shared<EventOutbox>(executor_, params.callbackUrl);
    poster->run();

    auto client_builder =
        launchdarkly::sse::Builder(executor_, params.streamUrl);

    if (params.headers) {
        for (auto const& header : *params.headers) {
            client_builder.header(header.first, header.second);
        }
    }

    if (params.method) {
        client_builder.method(http::string_to_verb(*params.method));
    }

    if (params.body) {
        client_builder.body(*params.body);
    }

    if (params.readTimeoutMs) {
        client_builder.read_timeout(
            std::chrono::milliseconds(*params.readTimeoutMs));
    }

    if (params.initialDelayMs) {
        client_builder.initial_reconnect_delay(
            std::chrono::milliseconds(*params.initialDelayMs));
    }

    // Use the global CURL setting from command line
    if (use_curl_) {
        client_builder.use_curl(true);
    }

    client_builder.logger([this](std::string msg) {
        LD_LOG(logger_, LogLevel::kDebug) << std::move(msg);
    });

    client_builder.receiver([copy = poster](launchdarkly::sse::Event event) {
        copy->post_event(std::move(event));
    });

    client_builder.errors([copy = poster](launchdarkly::sse::Error event) {
        copy->post_error(event);
    });

    auto client = client_builder.build();
    if (!client) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "entity_manager: couldn't build sse client";
        return std::nullopt;
    }

    client->async_connect();

    entities_.emplace(id, std::make_pair(client, poster));
    return id;
}

bool EntityManager::destroy(std::string const& id) {
    auto it = entities_.find(id);
    if (it == entities_.end()) {
        return false;
    }

    it->second.first->async_shutdown(nullptr);
    it->second.second->stop();

    entities_.erase(it);

    return true;
}
