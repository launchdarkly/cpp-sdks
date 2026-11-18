#include "entity_manager.hpp"
#include "event_outbox.hpp"

using launchdarkly::LogLevel;

EntityManager::EntityManager(boost::asio::any_io_executor executor,
                             launchdarkly::Logger& logger)
    : counter_{0}, executor_{std::move(executor)}, logger_{logger} {}

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

    client_builder.logger([this](std::string msg) {
        LD_LOG(logger_, LogLevel::kDebug) << std::move(msg);
    });

    auto suppress_next_error = std::make_shared<std::atomic_bool>(false);

    client_builder.receiver([copy = poster](launchdarkly::sse::Event event) {
        copy->post_event(std::move(event));
    });

    client_builder.errors(
        [copy = poster, suppress = suppress_next_error](launchdarkly::sse::Error event) {
            // A caller-triggered restart surfaces as an error on the read
            // path (the canceled read looks like a body timeout); like other
            // services (e.g. okhttp's StreamClosedByCallerException
            // suppression) we don't report it to the harness.
            if (suppress->exchange(false)) {
                return;
            }
            copy->post_error(event);
        });

    auto client = client_builder.build();
    if (!client) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "entity_manager: couldn't build sse client";
        return std::nullopt;
    }

    client->async_connect();

    entities_.emplace(id, std::make_tuple(client, poster, suppress_next_error));
    return id;
}

std::optional<bool> EntityManager::command(std::string const& id,
                                           std::string const& command) {
    auto it = entities_.find(id);
    if (it == entities_.end()) {
        return std::nullopt;
    }

    if (command == "restart") {
        std::get<2>(it->second)->store(true);
        std::get<0>(it->second)->async_restart("test harness restart command");
        return true;
    }

    return false;
}

bool EntityManager::destroy(std::string const& id) {
    auto it = entities_.find(id);
    if (it == entities_.end()) {
        return false;
    }

    std::get<0>(it->second)->async_shutdown(nullptr);
    std::get<1>(it->second)->stop();

    entities_.erase(it);

    return true;
}
