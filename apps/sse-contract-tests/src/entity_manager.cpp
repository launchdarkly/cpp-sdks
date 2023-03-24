#include "entity_manager.hpp"
#include "stream_entity.hpp"

using launchdarkly::LogLevel;

EntityManager::EntityManager(boost::asio::any_io_executor executor,
                             launchdarkly::Logger& logger)
    : entities_(),
      counter_{0},
      executor_{std::move(executor)},
      logger_{logger} {}

std::optional<std::string> EntityManager::create(ConfigParams params) {
    std::string id = std::to_string(counter_++);

    auto poster = std::make_shared<EventOutbox>(executor_, params.callbackUrl);
    poster->run();

    auto client_builder =
        launchdarkly::sse::Builder(executor_, params.streamUrl);

    if (params.headers) {
        for (auto const& h : *params.headers) {
            client_builder.header(h.first, h.second);
        }
    }

    client_builder.logging([this](std::string msg) {
        LD_LOG(logger_, LogLevel::kDebug) << std::move(msg);
    });

    client_builder.receiver([copy = poster](launchdarkly::sse::Event e) {
        copy->deliver_event(std::move(e));
    });

    auto client = client_builder.build();
    if (!client) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "entity_manager: couldn't build sse client";
        return std::nullopt;
    }

    client->run();

    entities_.emplace(id, std::make_pair(client, poster));
    return id;
}

void EntityManager::destroy_all() {
    for (auto& kv : entities_) {
        auto& entities = kv.second;
        // todo: entities.first.stop()
        entities.second->stop();
    }
    entities_.clear();
}

bool EntityManager::destroy(std::string const& id) {
    auto it = entities_.find(id);
    if (it == entities_.end()) {
        return false;
    }

    it->second.second->stop();

    entities_.erase(it);

    return true;
}
