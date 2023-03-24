#pragma once

#include "definitions.hpp"
#include "logger.hpp"

#include <boost/asio/any_io_executor.hpp>

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include <launchdarkly/sse/client.hpp>

class EventOutbox;

//
class EntityManager {
    using Inbox = std::shared_ptr<launchdarkly::sse::Client>;
    using Outbox = std::shared_ptr<EventOutbox>;
    using Entity = std::pair<Inbox, Outbox>;

    std::unordered_map<std::string, Entity> entities_;

    std::size_t counter_;
    boost::asio::any_io_executor executor_;

    launchdarkly::Logger& logger_;

   public:
    EntityManager(boost::asio::any_io_executor executor,
                  launchdarkly::Logger& logger);
    std::optional<std::string> create(ConfigParams params);
    bool destroy(std::string const& id);

    void destroy_all();

    friend class StreamEntity;
};
