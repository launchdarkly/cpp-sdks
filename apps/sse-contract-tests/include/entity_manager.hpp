#pragma once

#include "definitions.hpp"

#include <boost/asio/any_io_executor.hpp>

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class StreamEntity;

// Manages the individual SSE clients (called entities here) which are
// instantiated for each contract test.
class EntityManager {
    // Maps the entity's ID to the entity. Shared pointer is necessary because
    // these entities are doing async IO and must remain alive as long as that
    // is happening.
    std::unordered_map<std::string, std::weak_ptr<StreamEntity>> entities_;
    // Incremented each time create() is called to instantiate a new entity.
    std::size_t counter_;
    // Synchronizes access to create()/destroy();
    std::mutex lock_;
    boost::asio::any_io_executor executor_;

   public:
    explicit EntityManager(boost::asio::any_io_executor executor);
    std::optional<std::string> create(ConfigParams params);
    bool destroy(std::string const& id);

    void destroy_all();

    friend class StreamEntity;
};
