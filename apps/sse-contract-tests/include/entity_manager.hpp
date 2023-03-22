#pragma once

#include "definitions.hpp"
#include "stream_entity.hpp"

#include <boost/asio/executor.hpp>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <optional>
#include <string>

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
    explicit EntityManager(boost::asio::any_io_executor executor);
    std::optional<std::string> create(ConfigParams params);
    bool destroy(std::string const& id);
};
