#include "launchdarkly/client_side/api.hpp"
#include <chrono>
#include <optional>
#include <utility>

#include "launchdarkly/client_side/data_sources/detail/polling_data_source.hpp"
#include "launchdarkly/client_side/data_sources/detail/streaming_data_source.hpp"
#include "launchdarkly/client_side/event_processor/detail/event_processor.hpp"
#include "launchdarkly/client_side/event_processor/detail/null_event_processor.hpp"

namespace launchdarkly::client_side {

using launchdarkly::client_side::data_sources::DataSourceStatus;

static std::unique_ptr<IDataSource> MakeDataSource(
    Config const& config,
    Context const& context,
    boost::asio::any_io_executor const& executor,
    flag_manager::detail::FlagUpdater& flag_updater,
    data_sources::detail::DataSourceStatusManager& status_manager,
    Logger& logger) {
    if (config.DataSourceConfig().method.which() == 0) {
        // TODO: use initial reconnect delay.
        return std::make_unique<launchdarkly::client_side::data_sources::
                                    detail::StreamingDataSource>(
            config, executor, context, &flag_updater, status_manager, logger);
    }
    return std::make_unique<
        launchdarkly::client_side::data_sources::detail::PollingDataSource>(
        config, executor, context, &flag_updater, status_manager, logger);
}

Client::Client(Config config, Context context)
    : logger_(config.Logger()),
      context_(std::move(context)),
      event_processor_(nullptr),
      flag_updater_(flag_manager_),
      data_source_(MakeDataSource(config,
                                  context_,
                                  ioc_.get_executor(),
                                  flag_updater_,
                                  status_manager_,
                                  logger_)),
<<<<<<< HEAD
      initialized_(false) {
    if (config.Events().Enabled()) {
        event_processor_ = std::make_unique<detail::EventProcessor>(
            ioc_.get_executor(), config, logger_);
    } else {
        event_processor_ = std::make_unique<detail::NullEventProcessor>();
    }

    data_source_->Start();

||||||| parent of b4e9733 (feat: generate events from variation methods)
      initialized_(false) {
=======
      initialized_(false),
      eval_reasons_(config.DataSourceConfig().with_reasons) {
>>>>>>> b4e9733 (feat: generate events from variation methods)
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

    // Should listen to status before attempting to start.
    data_source_->Start();

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

EvaluationDetail Client::VariationInternal(FlagKey const& key,
                                           Value default_value,
                                           bool check_type) {
    auto desc = flag_manager_.Get(key);

    events::client::FeatureEventParams event = {
        std::chrono::system_clock::now(),
        key,
        context_,
        default_value,
        default_value,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        false,
        std::nullopt,
    };

    if (!desc || !desc->flag) {
        auto error_reason = EvaluationReason("FLAG_NOT_FOUND");
        if (eval_reasons_) {
            event.reason = error_reason;
        }
        event_processor_->AsyncSend(std::move(event));
        return EvaluationDetail(default_value, std::nullopt, error_reason);
    }

    auto const& flag = *(desc->flag);
    auto const& detail = flag.detail();

    if (check_type && default_value.type() != Value::Type::kNull &&
        detail.value().type() != default_value.type()) {
        auto error_reason = EvaluationReason("WRONG_TYPE");
        if (eval_reasons_) {
            event.reason = error_reason;
        }
        event_processor_->AsyncSend(std::move(event));
        return EvaluationDetail(default_value, std::nullopt, error_reason);
    }

    event.value = detail.value();
    event.variation = detail.variation_index();

    if (flag.track_reason()) {
        event.reason = detail.reason();
    }

    event.version = flag.flag_version().value_or(flag.version());
    event.require_full_event = flag.track_events();
    if (auto date = flag.debug_events_until_date()) {
        event.debug_events_until_date = events::Date{*date};
    }

    event_processor_->AsyncSend(std::move(event));

    return EvaluationDetail(detail.value(), detail.variation_index(),
                            detail.reason()->get());
}

bool Client::BoolVariation(Client::FlagKey const& key, bool default_value) {
    return VariationInternal(key, default_value, true).Value().as_bool();
}

std::pair<bool, EvaluationDetail> Client::BoolVariationDetail(
    Client::FlagKey const& key,
    bool default_value) {
    auto detail = VariationInternal(key, default_value, true);
    return {
        detail.Value().as_bool(),
        detail,
    };
}

std::string Client::StringVariation(Client::FlagKey const& key,
                                    std::string default_value) {
    return VariationInternal(key, std::move(default_value), true)
        .Value()
        .as_string();
}

double Client::DoubleVariation(Client::FlagKey const& key,
                               double default_value) {
    return VariationInternal(key, default_value, true).Value().as_double();
}

int Client::IntVariation(Client::FlagKey const& key, int default_value) {
    return VariationInternal(key, default_value, true).Value().as_int();
}

Value Client::JsonVariation(Client::FlagKey const& key, Value default_value) {
    return VariationInternal(key, std::move(default_value), false).Value();
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
