
#include <chrono>

#include <optional>
#include <utility>

#include "client_impl.hpp"
#include "data_sources/null_data_source.hpp"
#include "data_sources/polling_data_source.hpp"
#include "data_sources/streaming_data_source.hpp"

#include "event_processor/event_processor.hpp"
#include "event_processor/null_event_processor.hpp"

#include <launchdarkly/encoding/sha_256.hpp>
#include <launchdarkly/logging/console_backend.hpp>
#include <launchdarkly/logging/null_logger.hpp>

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

static std::shared_ptr<IDataSource> MakeDataSource(
    HttpProperties const& http_properties,
    std::optional<std::string> app_tags,
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

    // Event sources should include application tags.
    if (app_tags) {
        builder.Header("x-launchdarkly-tags", *app_tags);
    }

    auto data_source_properties = builder.Build();

    if (config.DataSourceConfig().method.index() == 0) {
        // TODO: use initial reconnect delay.
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

ClientImpl::ClientImpl(Config config,
                       Context context,
                       std::string const& version)
    : config_(config),
      http_properties_(HttpPropertiesBuilder(config.HttpProperties())
                           .Header("user-agent", "CPPClient/" + version)
                           .Header("authorization", config.SdkKey())
                           .Build()),
      logger_(MakeLogger(config.Logging())),
      ioc_(kAsioConcurrencyHint),
      context_(std::move(context)),
      flag_manager_(config.SdkKey(),
                    logger_,
                    config.Persistence().max_contexts_,
                    MakePersistence(config)),
      data_source_factory_([this]() {
          return MakeDataSource(http_properties_, config_.ApplicationTag(),
                                config_, context_, ioc_.get_executor(),
                                flag_manager_.Updater(), status_manager_,
                                logger_);
      }),
      data_source_(data_source_factory_()),
      event_processor_(nullptr),
      initialized_(false),
      eval_reasons_available_(config.DataSourceConfig().with_reasons) {
    flag_manager_.LoadCache(context_);

    if (config.Events().Enabled() && !config.Offline()) {
        event_processor_ = std::make_unique<EventProcessor>(
            ioc_.get_executor(), config.ServiceEndpoints(), config.Events(),
            http_properties_, logger_);
    } else {
        event_processor_ = std::make_unique<NullEventProcessor>();
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

    if (config.Offline()) {
        LD_LOG(logger_, LogLevel::kInfo)
            << "Starting LaunchDarkly client in offline mode";
    }

    // Should listen to status before attempting to start.
    data_source_->Start();

    run_thread_ = std::move(std::thread([&]() { ioc_.run(); }));

    event_processor_->SendAsync(events::client::IdentifyEventParams{
        std::chrono::system_clock::now(), context_});
}

bool ClientImpl::Initialized() const {
    std::unique_lock lock(init_mutex_);
    return initialized_;
}

std::unordered_map<Client::FlagKey, Value> ClientImpl::AllFlags() const {
    std::unordered_map<Client::FlagKey, Value> result;
    for (auto& [key, descriptor] : flag_manager_.Store().GetAll()) {
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

std::future<void> ClientImpl::IdentifyAsync(Context context) {
    flag_manager_.LoadCache(context);
    auto identify_promise = std::make_shared<std::promise<void>>();
    auto fut = identify_promise->get_future();
    data_source_->ShutdownAsync(
        [this, ctx = std::move(context), identify_promise]() {
            UpdateContextSynchronized(ctx);
            data_source_ = data_source_factory_();
            data_source_->Start();
            event_processor_->SendAsync(events::client::IdentifyEventParams{
                std::chrono::system_clock::now(), std::move(ctx)});
            identify_promise->set_value();
        });
    return fut;
}

// TODO(cwaldren): refactor VariationInternal so it isn't so long and mixing
// up multiple concerns.
template <typename T>
EvaluationDetail<T> ClientImpl::VariationInternal(FlagKey const& key,
                                                  Value default_value,
                                                  bool check_type,
                                                  bool detailed) {
    auto desc = flag_manager_.Store().Get(key);

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
        LD_LOG(logger_, LogLevel::kInfo)
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
    return flag_manager_.Notifier();
}

void ClientImpl::WaitForReadySync(std::chrono::milliseconds timeout) {
    std::unique_lock lock(init_mutex_);
    init_waiter_.wait_for(lock, timeout, [this] { return initialized_; });
}

void ClientImpl::UpdateContextSynchronized(Context context) {
    std::unique_lock lock(context_mutex_);
    context_ = std::move(context);
}

ClientImpl::~ClientImpl() {
    auto shutdown_promise = std::make_shared<std::promise<void>>();
    auto fut = shutdown_promise->get_future();

    data_source_->ShutdownAsync(
        [shutdown_promise]() { shutdown_promise->set_value(); });
    fut.wait_for(kDataSourceShutdownWait);

    ioc_.stop();
    // TODO: Probably not the best.
    run_thread_.join();
}

}  // namespace launchdarkly::client_side
