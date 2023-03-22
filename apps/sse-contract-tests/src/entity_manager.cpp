#include "entity_manager.hpp"

#include "launchdarkly/sse/sse.hpp"

EntityManager::EntityManager(boost::asio::any_io_executor executor)
    : entities_{}, counter_{0}, lock_{}, executor_{std::move(executor)} {}

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
    std::shared_ptr<launchdarkly::sse::client> client = client_builder.build();
    if (!client) {
        return std::nullopt;
    }
    std::shared_ptr<StreamEntity> entity =
        std::make_shared<StreamEntity>(executor_, client, params.callbackUrl);

    // Kicks off asynchronous operations.
    entity->run();
    entities_.emplace(id, entity);

    return id;
}

bool EntityManager::destroy(std::string const& id) {
    std::lock_guard<std::mutex> guard{lock_};
    auto it = entities_.find(id);
    if (it == entities_.end()) {
        return false;
    }
    // Shuts down asynchronous operations.
    it->second->stop();
    entities_.erase(it);
    return true;
}
