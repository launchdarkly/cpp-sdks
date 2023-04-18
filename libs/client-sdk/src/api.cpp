#include "launchdarkly/client_side/api.hpp"
#include <chrono>
#include <optional>
#include <utility>

#include "console_backend.hpp"
#include "events/client_events.hpp"
#include "events/detail/asio_event_processor.hpp"

namespace launchdarkly::client_side {

Client::Client(client::Config config, Context context)
    : logger_(config.Logger()),
      context_(std::move(context)),
      event_processor_(
          std::make_unique<launchdarkly::events::detail::AsioEventProcessor>(
              ioc_.get_executor(),
              config.Events(),
              config.ServiceEndpoints(),
              config.SdkKey(),
              logger_)) {}

std::unordered_map<Client::FlagKey, Value> Client::AllFlags() const {
    return {};
}

void Client::TrackInternal(std::string event_name,
                           std::optional<Value> data,
                           std::optional<double> metric_value) {
    event_processor_->AsyncSend(events::TrackEventParams{
        std::chrono::system_clock::now(), std::move(event_name),
        context_.kinds_to_keys(), std::move(data), metric_value});
}

void Client::Track(std::string event_name, Value data, double metric_value) {
    this->TrackInternal(std::move(event_name), data, metric_value);
}

void Client::Track(std::string event_name, Value data) {
    this->TrackInternal(std::move(event_name), data, std::nullopt);
}

void Client::Track(std::string event_name) {
    this->TrackInternal(std::move(event_name), std::nullopt, std::nullopt);
}

void Client::AsyncFlush() {
    event_processor_->AsyncFlush();
}

void Client::AsyncIdentify(Context context) {
    event_processor_->AsyncSend(events::client::IdentifyEventParams{
        std::chrono::system_clock::now(), std::move(context)});
}

bool Client::BoolVariation(Client::FlagKey const& key, bool default_value) {
    return default_value;
}

std::string Client::StringVariation(Client::FlagKey const& key,
                                    std::string default_value) {
    return default_value;
}

double Client::DoubleVariation(Client::FlagKey const& key,
                               double default_value) {
    return default_value;
}

int Client::IntVariation(Client::FlagKey const& key, int default_value) {
    return default_value;
}

Value Client::JsonVariation(Client::FlagKey const& key, Value default_value) {
    return default_value;
}

}  // namespace launchdarkly::client_side
