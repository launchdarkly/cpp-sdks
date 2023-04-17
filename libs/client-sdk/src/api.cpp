#include "launchdarkly/client_side/api.hpp"
#include <cstdint>
#include <optional>
#include "console_backend.hpp"
#include "events/detail/asio_event_processor.hpp"

namespace launchdarkly::client_side {

Client::Client(std::unique_ptr<ILogBackend> log_backend,
               std::string sdk_key,
               Context context,
               config::detail::Events const& event_config,
               config::ServiceEndpoints const& endpoints_config)
    : logger_(std::move(log_backend)),
      ioc_(),
      context_(std::move(context)),
      event_processor_(
          std::make_unique<launchdarkly::events::detail::AsioEventProcessor>(
              ioc_.get_executor(),
              event_config,
              endpoints_config,
              sdk_key,
              logger_)) {}

tl::expected<Client, Error> Create(client::Config config, Context context) {
    auto events = config.events_builder.build();
    if (!events) {
        return tl::make_unexpected(events.error());
    }

    auto endpoints = config.service_endpoints_builder.build();
    if (!endpoints) {
        return tl::make_unexpected(endpoints.error());
    }

    if (config.sdk_key.empty()) {
        return tl::make_unexpected(Error::kConfig_SDKKey_Empty);
    }

    std::unique_ptr<ILogBackend> log_backend =
        std::make_unique<ConsoleBackend>("LaunchDarklyClient");

    return {tl::in_place,
            std::move(log_backend),
            std::move(config.sdk_key),
            std::move(context),
            *events,
            *endpoints};
}
}  // namespace launchdarkly::client_side
