#include "launchdarkly/client_side/api.hpp"
#include <chrono>
#include <optional>
#include <utility>

#include "events/detail/asio_event_processor.hpp"
#include "launchdarkly/client_side/data_sources/detail/polling_data_source.hpp"
#include "launchdarkly/client_side/data_sources/detail/streaming_data_source.hpp"

namespace launchdarkly::client_side {

using launchdarkly::client_side::data_sources::DataSourceStatus;

static std::unique_ptr<IDataSource> MakeDataSource(
    Config const& config,
    Context const& context,
    boost::asio::any_io_executor executor,
    flag_manager::detail::FlagUpdater& flag_updater,
    data_sources::detail::DataSourceStatusManager& status_manager,
    Logger& logger) {
    if (config.DataSourceConfig().method.which() == 0) {
        // TODO: use initial reconnect delay.
        return std::make_unique<launchdarkly::client_side::data_sources::
                                    detail::StreamingDataSource>(
            config.SdkKey(), executor, context, config.ServiceEndpoints(),
            config.HttpProperties(), config.DataSourceConfig().use_report,
            config.DataSourceConfig().with_reasons, &flag_updater,
            status_manager, logger);
    } else {
        auto polling_config = boost::get<config::detail::built::PollingConfig>(
            config.DataSourceConfig().method);
        return std::make_unique<
            launchdarkly::client_side::data_sources::detail::PollingDataSource>(
            config.SdkKey(), executor, context, config.ServiceEndpoints(),
            config.HttpProperties(), polling_config.poll_interval,
            config.DataSourceConfig().use_report,
            config.DataSourceConfig().with_reasons, &flag_updater,
            status_manager, logger);
    }
}

Client::Client(Config config, Context context)
    : logger_(config.Logger()),
      context_(std::move(context)),
      event_processor_(
          std::make_unique<launchdarkly::events::detail::AsioEventProcessor>(
              ioc_.get_executor(),
              config.Events(),
              config.ServiceEndpoints(),
              config.SdkKey(),
              logger_)),
      flag_updater_(flag_manager_),
      data_source_(MakeDataSource(config,
                                  context_,
                                  ioc_.get_executor(),
                                  flag_updater_,
                                  status_manager_,
                                  logger_)),
      initialized_(false) {
    data_source_->Start();

    status_manager_.OnDataSourceStatusChange([this](auto status) {
        if (status.State() == DataSourceStatus::DataSourceState::kValid ||
            status.State() == DataSourceStatus::DataSourceState::kShutdown ||
            status.State() == DataSourceStatus::DataSourceState::kSetOffline) {
            {
                std::unique_lock lock(init_mutex_);
                initialized_ = true;
            }
            init_waiter_.notify_all();
        }
    });

    run_thread_ = std::move(std::thread([&]() { ioc_.run(); }));
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
    return VariationInternal(key, std::move(default_value)).as_string();
}

double Client::DoubleVariation(Client::FlagKey const& key,
                               double default_value) {
    return VariationInternal(key, default_value).as_double();
}

int Client::IntVariation(Client::FlagKey const& key, int default_value) {
    return VariationInternal(key, default_value).as_int();
}

Value Client::JsonVariation(Client::FlagKey const& key, Value default_value) {
    return VariationInternal(key, std::move(default_value));
}

data_sources::IDataSourceStatusProvider& Client::DataSourceStatus() {
    return status_manager_;
}

void Client::WaitForReadySync(std::chrono::seconds timeout) {
    std::unique_lock lock(init_mutex_);
    init_waiter_.wait_for(lock, timeout, [this] { return initialized_; });
}

Client::~Client() {
    data_source_->Close();
    ioc_.stop();
    // TODO: Probably not the best.
    run_thread_.join();
}

}  // namespace launchdarkly::client_side
