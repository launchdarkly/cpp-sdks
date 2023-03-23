#include "entity_manager.hpp"
#include "launchdarkly/sse/sse.hpp"
#include "stream_entity.hpp"

using launchdarkly::LogLevel;

EntityManager::EntityManager(boost::asio::any_io_executor executor,
                             launchdarkly::Logger& logger)
    : entities_{},
      counter_{0},
      lock_{},
      executor_{std::move(executor)},
      logger_{logger} {}

std::optional<std::string> EntityManager::create(ConfigParams params) {
    std::lock_guard<std::mutex> guard{lock_};
    std::string id = std::to_string(counter_++);

    auto client_builder =
        launchdarkly::sse::builder{executor_, params.streamUrl};
    if (params.headers) {
        for (auto h : *params.headers) {
            client_builder.header(h.first, h.second);
        }
    }

    client_builder.logging([this](std::string msg){
        LD_LOG(logger_, LogLevel::kDebug) << std::move(msg);
    });

    std::shared_ptr<launchdarkly::sse::client> client = client_builder.build();
    if (!client) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "entity_manager: couldn't build sse client";
        return std::nullopt;
    }
    std::shared_ptr<StreamEntity> entity =
        std::make_shared<StreamEntity>(executor_, client, params.callbackUrl);

    entity->run();

    entities_.emplace(id, entity);
    return id;
}

void EntityManager::destroy_all() {
    for (auto& entity : entities_) {
        if (auto weak = entity.second.lock()) {
            weak->stop();
        }
    }
    entities_.clear();
}

bool EntityManager::destroy(std::string const& id) {
    std::lock_guard<std::mutex> guard{lock_};
    auto it = entities_.find(id);
    if (it == entities_.end()) {
        return false;
    }

    if (auto weak = it->second.lock()) {
        weak->stop();
    }

    entities_.erase(it);

    return true;
}
