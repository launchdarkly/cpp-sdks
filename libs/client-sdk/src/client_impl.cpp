
#include <chrono>
#include <optional>
#include <utility>

#include "client_impl.hpp"
#include "data_sources/polling_data_source.hpp"
#include "data_sources/streaming_data_source.hpp"
#include "event_processor/event_processor.hpp"
#include "event_processor/null_event_processor.hpp"

#include <launchdarkly/config/shared/built/logging.hpp>
#include <launchdarkly/logging/console_backend.hpp>
#include <launchdarkly/logging/null_logger.hpp>

namespace launchdarkly::client_side {

// The ASIO implementation assumes that the io_context will be run from a
// single thread, and applies several optimisations based on this assumption.
auto const kAsioConcurrencyHint = 1;

// Client's destructor attempts to gracefully shut down the datasource
// connection in this amount of time.
auto const kDataSourceShutdownWait = std::chrono::milliseconds(100);

using launchdarkly::client_side::data_sources::DataSourceStatus;

static std::shared_ptr<IDataSource> MakeDataSource(
    Config const& config,
    Context const& context,
    boost::asio::any_io_executor const& executor,
    flag_manager::FlagUpdater& flag_updater,
    data_sources::DataSourceStatusManager& status_manager,
    Logger& logger) {
    if (config.DataSourceConfig().method.index() == 0) {
        // TODO: use initial reconnect delay.
        return std::make_shared<
            launchdarkly::client_side::data_sources::StreamingDataSource>(
            config, executor, context, &flag_updater, status_manager, logger);
    }
    return std::make_shared<
        launchdarkly::client_side::data_sources::PollingDataSource>(
        config, executor, context, &flag_updater, status_manager, logger);
}

static Logger MakeLogger(config::shared::built::Logging const& config) {
    if (config.disable_logging) {
        return {std::make_shared<logging::NullLoggerBackend>()};
    }
    if (config.backend) {
        return {config.backend};
    }
    return {
        std::make_shared<logging::ConsoleBackend>(config.level, config.tag)};
}

ClientImpl::ClientImpl(Config config, Context context)
    : config_(config),
      logger_(MakeLogger(config.Logging())),
      ioc_(kAsioConcurrencyHint),
      work_(boost::asio::make_work_guard(ioc_)),
      context_(std::move(context)),
      data_source_factory_([this]() {
          return MakeDataSource(config_, context_, ioc_.get_executor(),
                                flag_updater_, status_manager_, logger_);
      }),
      data_source_(nullptr),
      event_processor_(nullptr),
      flag_updater_(flag_manager_),
      initialized_(false),
      eval_reasons_available_(config.DataSourceConfig().with_reasons) {
    if (config.Events().Enabled()) {
        event_processor_ = std::make_unique<EventProcessor>(ioc_.get_executor(),
                                                            config, logger_);
    } else {
        event_processor_ = std::make_unique<NullEventProcessor>();
    }

    event_processor_->SendAsync(events::client::IdentifyEventParams{
        std::chrono::system_clock::now(), context_});

    run_thread_ = std::move(std::thread([&]() { ioc_.run(); }));
}

std::future<void> ClientImpl::IdentifyAsync(Context context) {
    UpdateContextSynchronized(context);
    event_processor_->SendAsync(events::client::IdentifyEventParams{
        std::chrono::system_clock::now(), std::move(context)});

    return RunAsyncInternal([](auto state) {
        return (state == DataSourceStatus::DataSourceState::kValid ||
                state == DataSourceStatus::DataSourceState::kShutdown ||
                state == DataSourceStatus::DataSourceState::kSetOffline);
    });
}

void ClientImpl::RestartDataSource() {
    auto start_op = [this]() {
        data_source_ = data_source_factory_();
        data_source_->Start();
    };
    if (!data_source_) {
        return start_op();
    }
    data_source_->ShutdownAsync(start_op);
}

std::future<void> ClientImpl::RunAsyncInternal(
    std::function<bool(DataSourceStatus::DataSourceState)> cond) {
    auto pr = std::make_shared<std::promise<void>>();
    auto fut = pr->get_future();

    // OK the problem here is that the datasource never becomes valid, hence
    // Identify never returns.
    // WHy ? because the test aharness is sending bogus data.So what are we
    //       supposed to do herE
    status_manager_.OnDataSourceStatusChangeEx(
        [this, cond, pr](data_sources::DataSourceStatus status) {
            if (cond(status.State())) {
                LD_LOG(logger_, LogLevel::kInfo)
                    << "Calling future set value: " << status.State();
                pr->set_value();
                return true;
            }
            return false;
        });

    RestartDataSource();

    return fut;
}

std::future<void> ClientImpl::RunAsync() {
    return RunAsyncInternal([](auto state) {
        return (state == DataSourceStatus::DataSourceState::kValid ||
                state == DataSourceStatus::DataSourceState::kShutdown ||
                state == DataSourceStatus::DataSourceState::kSetOffline);
    });
}

bool ClientImpl::Initialized() const {
    return status_manager_.Status().State() ==
           DataSourceStatus::DataSourceState::kValid;
}

std::unordered_map<Client::FlagKey, Value> ClientImpl::AllFlags() const {
    std::unordered_map<Client::FlagKey, Value> result;
    for (auto& [key, descriptor] : flag_manager_.GetAll()) {
        if (descriptor->flag) {
            result.try_emplace(key, descriptor->flag->detail().value());
        }
    }
    return result;
}

void ClientImpl::TrackInternal(std::string event_name,
                               std::optional<Value> data,
                               std::optional<double> metric_value) {
    event_processor_->SendAsync(events::TrackEventParams{
        std::chrono::system_clock::now(), std::move(event_name),
        ReadContextSynchronized(
            [](Context const& c) { return c.kinds_to_keys(); }),
        std::move(data), metric_value});
}

void ClientImpl::Track(std::string event_name,
                       Value data,
                       double metric_value) {
    this->TrackInternal(std::move(event_name), std::move(data), metric_value);
}

void ClientImpl::Track(std::string event_name, Value data) {
    this->TrackInternal(std::move(event_name), std::move(data), std::nullopt);
}

void ClientImpl::Track(std::string event_name) {
    this->TrackInternal(std::move(event_name), std::nullopt, std::nullopt);
}

void ClientImpl::FlushAsync() {
    event_processor_->FlushAsync();
}

// TODO(cwaldren): refactor VariationInternal so it isn't so long and mixing up
// multiple concerns.
template <typename T>
EvaluationDetail<T> ClientImpl::VariationInternal(FlagKey const& key,
                                                  Value default_value,
                                                  bool check_type,
                                                  bool detailed) {
    auto desc = flag_manager_.Get(key);

    events::client::FeatureEventParams event = {
        std::chrono::system_clock::now(),
        key,
        ReadContextSynchronized([](Context const& c) { return c; }),
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
            event_processor_->SendAsync(std::move(event));
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
        event_processor_->SendAsync(std::move(event));
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

    if (check_type && default_value.Type() != Value::Type::kNull &&
        detail.value().Type() != default_value.Type()) {
        auto error_reason =
            EvaluationReason(EvaluationReason::ErrorKind::kWrongType);
        if (eval_reasons_available_) {
            event.reason = error_reason;
        }
        event_processor_->SendAsync(std::move(event));
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

    event_processor_->SendAsync(std::move(event));

    return EvaluationDetail<T>(detail.value(), detail.variation_index(),
                               detail.reason());
}

EvaluationDetail<bool> ClientImpl::BoolVariationDetail(
    IClient::FlagKey const& key,
    bool default_value) {
    return VariationInternal<bool>(key, default_value, true, true);
}

bool ClientImpl::BoolVariation(IClient::FlagKey const& key,
                               bool default_value) {
    return *VariationInternal<bool>(key, default_value, true, false);
}

EvaluationDetail<std::string> ClientImpl::StringVariationDetail(
    ClientImpl::FlagKey const& key,
    std::string default_value) {
    return VariationInternal<std::string>(key, std::move(default_value), true,
                                          true);
}

std::string ClientImpl::StringVariation(IClient::FlagKey const& key,
                                        std::string default_value) {
    return *VariationInternal<std::string>(key, std::move(default_value), true,
                                           false);
}

EvaluationDetail<double> ClientImpl::DoubleVariationDetail(
    ClientImpl::FlagKey const& key,
    double default_value) {
    return VariationInternal<double>(key, default_value, true, true);
}

double ClientImpl::DoubleVariation(IClient::FlagKey const& key,
                                   double default_value) {
    return *VariationInternal<double>(key, default_value, true, false);
}

EvaluationDetail<int> ClientImpl::IntVariationDetail(
    IClient::FlagKey const& key,
    int default_value) {
    return VariationInternal<int>(key, default_value, true, true);
}

int ClientImpl::IntVariation(IClient::FlagKey const& key, int default_value) {
    return *VariationInternal<int>(key, default_value, true, false);
}

EvaluationDetail<Value> ClientImpl::JsonVariationDetail(
    IClient::FlagKey const& key,
    Value default_value) {
    return VariationInternal<Value>(key, std::move(default_value), false, true);
}

Value ClientImpl::JsonVariation(IClient::FlagKey const& key,
                                Value default_value) {
    return *VariationInternal<Value>(key, std::move(default_value), false,
                                     false);
}

data_sources::IDataSourceStatusProvider& ClientImpl::DataSourceStatus() {
    return status_manager_;
}

flag_manager::IFlagNotifier& ClientImpl::FlagNotifier() {
    return flag_updater_;
}

void ClientImpl::UpdateContextSynchronized(Context context) {
    std::unique_lock lock(context_mutex_);
    context_ = std::move(context);
}

ClientImpl::~ClientImpl() {
    ioc_.stop();
    // TODO: Probably not the best.
    run_thread_.join();
}

}  // namespace launchdarkly::client_side
