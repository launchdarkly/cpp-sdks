#pragma once

#include "client_entity.hpp"

#include <boost/asio/any_io_executor.hpp>

#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/server_side/client.hpp>

#include <data_model/data_model.hpp>

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class EventOutbox;

class EntityManager {
    std::unordered_map<std::string, ClientEntity> entities_;

    std::size_t counter_;
    boost::asio::any_io_executor executor_;

    launchdarkly::Logger& logger_;

   public:
    /**
     * Create an entity manager, which can be used to create and destroy
     * entities (SSE clients + event channel back to test harness).
     * @param executor Executor.
     * @param logger Logger.
     */
    EntityManager(boost::asio::any_io_executor executor,
                  launchdarkly::Logger& logger);
    /**
     * Create an entity with the given configuration.
     * @param params Config of the entity.
     * @return An ID representing the entity, or none if the entity couldn't
     * be created.
     */
    std::optional<std::string> create(ConfigParams const& params);
    /**
     * Destroy an entity with the given ID.
     * @param id ID of the entity.
     * @return True if the entity was found and destroyed.
     */
    bool destroy(std::string const& id);

    tl::expected<nlohmann::json, std::string> command(
        std::string const& id,
        CommandParams const& params);
};
