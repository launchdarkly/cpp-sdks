#include "launchdarkly/client_side/api.hpp"
#include <chrono>
#include <optional>
#include <utility>

#include "console_backend.hpp"
#include "events/client_events.hpp"
#include "events/detail/asio_event_processor.hpp"
#include "launchdarkly/client_side/data_sources/detail/streaming_data_source.hpp"

namespace launchdarkly::client_side {

Client::Client(client::Config config, Context context)
    : logger_(config.take_logger()),
      context_(std::move(context)),
      event_processor_(
          std::make_unique<launchdarkly::events::detail::AsioEventProcessor>(
              ioc_.get_executor(),
              config.events_config(),
              config.service_endpoints(),
              config.sdk_key(),
              logger_)),
      flag_updater_(flag_manager_),
      // TODO: Support polling.
      data_source_(std::make_unique<launchdarkly::client_side::data_sources::
                                        detail::StreamingDataSource>(
          config.sdk_key(),
          ioc_.get_executor(),
          context_,
          config.service_endpoints(),
          config.http_properties(),
          config.data_source_config().use_report,
          config.data_source_config().with_reasons,
          &flag_updater_,
          logger_)) {
    data_source_->Start();

    // TODO: This isn't a long-term solution.
    std::thread([&](){
        ioc_.run();
    }).detach();
}

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

Value Client::VariationInternal(FlagKey const& key, Value default_value) {
    auto res = flag_manager_.Get(key);
    if (res && res->flag) {
        return res->flag->detail().value();
    }
    return default_value;
}

bool Client::BoolVariation(Client::FlagKey const& key, bool default_value) {
    return VariationInternal(key, default_value).as_bool();
}

std::string Client::StringVariation(Client::FlagKey const& key,
                                    std::string default_value) {
    return VariationInternal(key, default_value).as_string();
}

double Client::DoubleVariation(Client::FlagKey const& key,
                               double default_value) {
    return VariationInternal(key, default_value).as_double();
}

int Client::IntVariation(Client::FlagKey const& key, int default_value) {
    return VariationInternal(key, default_value).as_int();
}

Value Client::JsonVariation(Client::FlagKey const& key, Value default_value) {
    return VariationInternal(key, default_value);
}

}  // namespace launchdarkly::client_side
