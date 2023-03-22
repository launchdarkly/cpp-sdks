#pragma once

#include "definitions.hpp"
#include "stream_entity.hpp"

#include <memory>
#include <mutex>
#include <unordered_map>

using namespace launchdarkly::sse;

// Manages the individual SSE clients (called entities here) which are
// instantiated for each contract test.
class EntityManager {
    // Maps the entity's ID to the entity. Shared pointer is necessary because
    // these entities are doing async IO and must remain alive as long as that
    // is happening.
    std::unordered_map<std::string, std::shared_ptr<stream_entity>> entities_;
    // Incremented each time create() is called to instantiate a new entity.
    std::size_t counter_;
    // Synchronizes access to create()/destroy();
    std::mutex lock_;
    boost::asio::any_io_executor executor_;

   public:
    explicit EntityManager(boost::asio::any_io_executor executor)
        : entities_{}, counter_{0}, lock_{}, executor_{std::move(executor)} {}

    std::optional<std::string> create(ConfigParams params) {
        std::lock_guard<std::mutex> guard{lock_};
        std::string id = std::to_string(counter_++);

        auto client_builder = builder{executor_, params.streamUrl};
        if (params.headers) {
            for (auto h : *params.headers) {
                client_builder.header(h.first, h.second);
            }
        }
        std::shared_ptr<client> client = client_builder.build();
        if (!client) {
            return std::nullopt;
        }
        std::shared_ptr<stream_entity> entity = std::make_shared<stream_entity>(
            executor_, client, params.callbackUrl);

        // Kicks off asynchronous operations.
        entity->run();
        entities_.emplace(id, entity);

        return id;
    }

    bool destroy(std::string const& id) {
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
};
