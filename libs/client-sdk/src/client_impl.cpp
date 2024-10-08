#include "client_impl.hpp"
#include "data_sources/null_data_source.hpp"
#include "data_sources/polling_data_source.hpp"
#include "data_sources/streaming_data_source.hpp"

#include <launchdarkly/events/asio_event_processor.hpp>
#include <launchdarkly/events/null_event_processor.hpp>

#include <launchdarkly/detail/c_binding_helpers.hpp>
#include <launchdarkly/encoding/sha_256.hpp>
#include <launchdarkly/logging/console_backend.hpp>
#include <launchdarkly/logging/null_logger.hpp>

#include <chrono>
#include <optional>
#include <utility>

namespace launchdarkly::client_side {

// The ASIO implementation assumes that the io_context will be run from a
// single thread, and applies several optimisations based on this
// assumption.
auto const kAsioConcurrencyHint = 1;

// Client's destructor attempts to gracefully shut down the datasource
// connection in this amount of time.
auto const kDataSourceShutdownWait = std::chrono::milliseconds(100);

using config::shared::ClientSDK;
using launchdarkly::client_side::data_sources::DataSourceStatus;
using launchdarkly::config::shared::built::DataSourceConfig;
using launchdarkly::config::shared::built::HttpProperties;

static std::shared_ptr<data_sources::IDataSource> MakeDataSource(
    HttpProperties const& http_properties,
    Config const& config,
    Context const& context,
    boost::asio::any_io_executor const& executor,
    IDataSourceUpdateSink& flag_updater,
    data_sources::DataSourceStatusManager& status_manager,
    Logger& logger) {
    if (config.Offline()) {
        return std::make_shared<data_sources::NullDataSource>(executor,
                                                              status_manager);
    }

    auto builder = HttpPropertiesBuilder(http_properties);

    auto data_source_properties = builder.Build();

    if (config.DataSourceConfig().method.index() == 0) {
        return std::make_shared<
            launchdarkly::client_side::data_sources::StreamingDataSource>(
            config.ServiceEndpoints(), config.DataSourceConfig(),
            data_source_properties, executor, context, flag_updater,
            status_manager, logger);
    }
    return std::make_shared<
        launchdarkly::client_side::data_sources::PollingDataSource>(
        config.ServiceEndpoints(), config.DataSourceConfig(),
        data_source_properties, executor, context, flag_updater, status_manager,
        logger);
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

static std::shared_ptr<IPersistence> MakePersistence(Config const& config) {
    auto persistence = config.Persistence();
    if (persistence.disable_persistence) {
        return nullptr;
    }
    return persistence.implementation;
}

ClientImpl::ClientImpl(Config in_cfg,
                       Context context,
                       std::string const& version)
    : config_(std::move(in_cfg)), /* caution: do not use in_cfg (moved from!) */
      http_properties_(
          HttpPropertiesBuilder(config_.HttpProperties())
              .Header("user-agent", "CPPClient/" + version)
              .Header("authorization", config_.SdkKey())
              .Header("x-launchdarkly-tags", config_.ApplicationTag())
              .Build()),
      logger_(MakeLogger(config_.Logging())),
      ioc_(kAsioConcurrencyHint),
      work_(boost::asio::make_work_guard(ioc_)),
      context_(std::move(context)),
      flag_manager_(config_.SdkKey(),
                    logger_,
                    config_.Persistence().max_contexts_,
                    MakePersistence(config_)),
      data_source_factory_([this]() {
          return MakeDataSource(http_properties_, config_, context_,
                                ioc_.get_executor(), flag_manager_.Updater(),
                                status_manager_, logger_);
      }),
      data_source_(nullptr),
      event_processor_(nullptr),
      eval_reasons_available_(config_.DataSourceConfig().with_reasons) {
    flag_manager_.LoadCache(context_);

    if (auto custom_ca = http_properties_.Tls().CustomCAFile()) {
        LD_LOG(logger_, LogLevel::kInfo)
            << "TLS peer verification configured with custom CA file: "
            << *custom_ca;
    }
    if (http_properties_.Tls().PeerVerifyMode() ==
        config::shared::built::TlsOptions::VerifyMode::kVerifyNone) {
        LD_LOG(logger_, LogLevel::kInfo) << "TLS peer verification disabled";
    }

    if (config_.Events().Enabled() && !config_.Offline()) {
        event_processor_ =
            std::make_unique<events::AsioEventProcessor<ClientSDK>>(
                ioc_.get_executor(), config_.ServiceEndpoints(),
                config_.Events(), http_properties_, logger_);
    } else {
        event_processor_ = std::make_unique<events::NullEventProcessor>();
    }

    event_processor_->SendAsync(events::IdentifyEventParams{
        std::chrono::system_clock::now(), context_});

    run_thread_ = std::move(std::thread([&]() { ioc_.run(); }));
}

// Returns true if the SDK can be considered initialized. We have defined
// explicit configuration of offline mode as initialized.
//
// When online, we're initialized if we've obtained a payload and are healthy
// (kValid) or obtained a payload and are unhealthy (kInterrupted).
//
// The purpose of this concept is to enable:
//
// (1) Resolving the StartAsync() promise. Once the SDK is no longer
// initializing, this promise needs to indicate if the process was successful
// or not.
//
// (2) Providing a getter for (1), if the user didn't check the promise or
// otherwise need to poll the state. That's the Initialized() method.
//
// (3) As a diagnostic during evaluation, to log a message warning that a
// cached (if persistence is being used) or default (if not) value will be
// returned because the SDK is not yet initialized.
static bool IsInitializedSuccessfully(DataSourceStatus::DataSourceState state) {
    return (state == DataSourceStatus::DataSourceState::kValid ||
            state == DataSourceStatus::DataSourceState::kSetOffline ||
            state == DataSourceStatus::DataSourceState::kInterrupted);
}

std::future<bool> ClientImpl::IdentifyAsync(Context context) {
    UpdateContextSynchronized(context);
    flag_manager_.LoadCache(context);
    event_processor_->SendAsync(events::IdentifyEventParams{
        std::chrono::system_clock::now(), std::move(context)});

    return StartAsyncInternal();
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

std::future<bool> ClientImpl::StartAsyncInternal() {
    auto init_promise = std::make_shared<std::promise<bool>>();
    auto init_future = init_promise->get_future();

    status_manager_.OnDataSourceStatusChangeEx(
        [init_promise](data_sources::DataSourceStatus const& status) {
            if (auto const state = status.State();
                state != DataSourceStatus::DataSourceState::kInitializing) {
                init_promise->set_value(
                    IsInitializedSuccessfully(status.State()));
                return true; /* delete this change listener since the desired
                                state was reached */
            }
            return false; /* keep the change listener */
        });

    RestartDataSource();

    return init_future;
}

std::future<bool> ClientImpl::StartAsync() {
    return StartAsyncInternal();
}

bool ClientImpl::Initialized() const {
    return IsInitializedSuccessfully(status_manager_.Status().State());
}

std::unordered_map<Client::FlagKey, Value> ClientImpl::AllFlags() const {
    std::unordered_map<Client::FlagKey, Value> result;
    for (auto& [key, descriptor] : flag_manager_.Store().GetAll()) {
        if (descriptor->item) {
            result.try_emplace(key, descriptor->item->Detail().Value());
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
            [](Context const& c) { return c.KindsToKeys(); }),
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

template <typename T>
EvaluationDetail<T> ClientImpl::VariationInternal(FlagKey const& key,
                                                  Value default_value,
                                                  bool check_type,
                                                  bool detailed) {
    auto desc = flag_manager_.Store().Get(key);

    events::FeatureEventParams event = {
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

    if (!desc || !desc->item) {
        if (!Initialized()) {
            LD_LOG(logger_, LogLevel::kWarn)
                << "LaunchDarkly client has not yet been initialized. "
                   "Returning default value";

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
    }

    if (!Initialized()) {
        LD_LOG(logger_, LogLevel::kInfo)
            << "LaunchDarkly client has not yet been initialized. "
               "Returning cached value";
    }

    LD_ASSERT(desc->item);

    auto const& flag = *(desc->item);
    auto const& detail = flag.Detail();

    if (check_type && default_value.Type() != Value::Type::kNull &&
        detail.Value().Type() != default_value.Type()) {
        auto error_reason =
            EvaluationReason(EvaluationReason::ErrorKind::kWrongType);
        if (eval_reasons_available_) {
            event.reason = error_reason;
        }
        event_processor_->SendAsync(std::move(event));
        return EvaluationDetail<T>(default_value, std::nullopt, error_reason);
    }

    event.value = detail.Value();
    event.variation = detail.VariationIndex();

    if (detailed || flag.TrackReason()) {
        event.reason = detail.Reason();
    }

    event.version = flag.FlagVersion().value_or(flag.Version());
    event.require_full_event = flag.TrackEvents();
    if (auto date = flag.DebugEventsUntilDate()) {
        event.debug_events_until_date = events::Date{*date};
    }

    event_processor_->SendAsync(std::move(event));

    return EvaluationDetail<T>(detail.Value(), detail.VariationIndex(),
                               detail.Reason());
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
    return flag_manager_.Notifier();
}

void ClientImpl::UpdateContextSynchronized(Context context) {
    std::unique_lock lock(context_mutex_);
    context_ = std::move(context);
}

ClientImpl::~ClientImpl() {
    ioc_.stop();
    // TODO(SC-219101)
    run_thread_.join();
}

}  // namespace launchdarkly::client_side
