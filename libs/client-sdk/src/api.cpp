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
      initialized_(false),
      eval_reasons_available_(config.DataSourceConfig().with_reasons) {
    if (config.Events().Enabled()) {
        event_processor_ = std::make_unique<detail::EventProcessor>(
            ioc_.get_executor(), config, logger_);
    } else {
        event_processor_ = std::make_unique<detail::NullEventProcessor>();
    }

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

bool Client::Initialized() const {
    std::unique_lock lock(init_mutex_);
    return initialized_;
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

// TODO(cwaldren): refactor VariationInternal so it isn't so long and mixing up
// multiple concerns.
template <typename T>
EvaluationDetail<T> Client::VariationInternal(FlagKey const& key,
                                              Value default_value,
                                              bool check_type,
                                              bool detailed) {
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
        if (!Initialized()) {
            LD_LOG(logger_, LogLevel::kWarn)
                << "LaunchDarkly client has not yet been initialized. "
                   "Returning default value";

            // TODO: SC-199918
            auto error_reason =
                EvaluationReason(EvaluationReason::ErrorKind::kClientNotReady);
            if (eval_reasons_available_) {
                event.reason = error_reason;
            }
            event_processor_->AsyncSend(std::move(event));
            return EvaluationDetail<T>(default_value, std::nullopt,
                                       std::move(error_reason));
        }

        LD_LOG(logger_, LogLevel::kInfo)
            << "Unknown feature flag " << key << "; returning default value";

        auto error_reason =
            EvaluationReason(EvaluationReason::ErrorKind::kFlagNotFound);
        if (eval_reasons_available_) {
            event.reason = error_reason;
        }
        event_processor_->AsyncSend(std::move(event));
        return EvaluationDetail<T>(default_value, std::nullopt,
                                   std::move(error_reason));

    } else if (!Initialized()) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "LaunchDarkly client has not yet been initialized. "
               "Returning cached value";
    }

    assert(desc->flag);

    auto const& flag = *(desc->flag);
    auto const& detail = flag.detail();

    if (check_type && default_value.type() != Value::Type::kNull &&
        detail.value().type() != default_value.type()) {
        auto error_reason =
            EvaluationReason(EvaluationReason::ErrorKind::kWrongType);
        if (eval_reasons_available_) {
            event.reason = error_reason;
        }
        event_processor_->AsyncSend(std::move(event));
        return EvaluationDetail<T>(default_value, std::nullopt, error_reason);
    }

    event.value = detail.value();
    event.variation = detail.variation_index();

    if (detailed || flag.track_reason()) {
        event.reason = detail.reason();
    }

    event.version = flag.flag_version().value_or(flag.version());
    event.require_full_event = flag.track_events();
    if (auto date = flag.debug_events_until_date()) {
        event.debug_events_until_date = events::Date{*date};
    }

    event_processor_->AsyncSend(std::move(event));

    return EvaluationDetail<T>(detail.value(), detail.variation_index(),
                               detail.reason());
}

EvaluationDetail<bool> Client::BoolVariationDetail(Client::FlagKey const& key,
                                                   bool default_value) {
    return VariationInternal<bool>(key, default_value, true, true);
}

bool Client::BoolVariation(Client::FlagKey const& key, bool default_value) {
    return *VariationInternal<bool>(key, default_value, true, false);
}

EvaluationDetail<std::string> Client::StringVariationDetail(
    Client::FlagKey const& key,
    std::string default_value) {
    return VariationInternal<std::string>(key, std::move(default_value), true,
                                          true);
}

std::string Client::StringVariation(Client::FlagKey const& key,
                                    std::string default_value) {
    return *VariationInternal<std::string>(key, std::move(default_value), true,
                                           false);
}

EvaluationDetail<double> Client::DoubleVariationDetail(
    Client::FlagKey const& key,
    double default_value) {
    return VariationInternal<double>(key, default_value, true, true);
}

double Client::DoubleVariation(Client::FlagKey const& key,
                               double default_value) {
    return *VariationInternal<double>(key, default_value, true, false);
}

EvaluationDetail<int> Client::IntVariationDetail(Client::FlagKey const& key,
                                                 int default_value) {
    return VariationInternal<int>(key, default_value, true, true);
}
int Client::IntVariation(Client::FlagKey const& key, int default_value) {
    return *VariationInternal<int>(key, default_value, true, false);
}

EvaluationDetail<Value> Client::JsonVariationDetail(Client::FlagKey const& key,
                                                    Value default_value) {
    return VariationInternal<Value>(key, std::move(default_value), false, true);
}

Value Client::JsonVariation(Client::FlagKey const& key, Value default_value) {
    return *VariationInternal<Value>(key, std::move(default_value), false,
                                     false);
}

data_sources::IDataSourceStatusProvider& Client::DataSourceStatus() {
    return status_manager_;
}

flag_manager::detail::IFlagNotifier& Client::FlagNotifier() {
    return flag_updater_;
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
