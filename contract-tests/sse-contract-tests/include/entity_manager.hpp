#pragma once

#include <launchdarkly/logging/logger.hpp>
#include "definitions.hpp"

#include <launchdarkly/sse/client.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class EventOutbox;

class EntityManager {
    using Inbox = std::shared_ptr<launchdarkly::sse::Client>;
    using Outbox = std::shared_ptr<EventOutbox>;
    using Entity = std::pair<Inbox, Outbox>;

    std::unordered_map<std::string, Entity> entities_;

    std::size_t counter_;
    boost::asio::any_io_executor executor_;

    launchdarkly::Logger& logger_;
    bool use_curl_;

   public:
    /**
     * Create an entity manager, which can be used to create and destroy
     * entities (SSE clients + event channel back to test harness).
     * @param executor Executor.
     * @param logger Logger.
     * @param use_curl Whether to use CURL implementation for SSE clients.
     */
    EntityManager(boost::asio::any_io_executor executor,
                  launchdarkly::Logger& logger,
                  bool use_curl = false);
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
};
