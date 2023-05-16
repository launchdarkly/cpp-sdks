
#include <chrono>
#include <optional>
#include <utility>

#include <launchdarkly/client_side/data_sources/detail/polling_data_source.hpp>
#include <launchdarkly/client_side/data_sources/detail/streaming_data_source.hpp>
#include <launchdarkly/client_side/detail/client_impl.hpp>
#include <launchdarkly/client_side/event_processor/detail/event_processor.hpp>
#include <launchdarkly/client_side/event_processor/detail/null_event_processor.hpp>

namespace launchdarkly::client_side::detail {

// The ASIO implementation assumes that the io_context will be run from a
// single thread, and applies several optimisations based on this assumption.
auto const kAsioConcurrencyHint = 1;

using launchdarkly::client_side::data_sources::DataSourceStatus;

static std::shared_ptr<IDataSource> MakeDataSource(
    Config const& config,
    Context const& context,
    boost::asio::any_io_executor const& executor,
    flag_manager::detail::FlagUpdater& flag_updater,
    data_sources::detail::DataSourceStatusManager& status_manager,
    Logger& logger) {
    if (config.DataSourceConfig().method.index() == 0) {
        // TODO: use initial reconnect delay.
        return std::make_shared<launchdarkly::client_side::data_sources::
                                    detail::StreamingDataSource>(
            config, executor, context, &flag_updater, status_manager, logger);
    }
    return std::make_shared<
        launchdarkly::client_side::data_sources::detail::PollingDataSource>(
        config, executor, context, &flag_updater, status_manager, logger);
}

ClientImpl::ClientImpl(Config config, Context context)
    : config_(config),
      logger_(config.Logger()),
      ioc_(kAsioConcurrencyHint),
      context_(std::move(context)),
      data_source_factory_([this]() {
          return MakeDataSource(config_, context_, ioc_.get_executor(),
                                flag_updater_, status_manager_, logger_);
      }),
      data_source_(data_source_factory_()),
      event_processor_(nullptr),
      flag_updater_(flag_manager_),
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

    event_processor_->AsyncSend(events::client::IdentifyEventParams{
        std::chrono::system_clock::now(), context_});
}

bool ClientImpl::Initialized() const {
    std::unique_lock lock(init_mutex_);
    return initialized_;
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
    event_processor_->AsyncSend(events::TrackEventParams{
        std::chrono::system_clock::now(), std::move(event_name),
        ReadContext<std::map<std::string, std::string>>(
            [](Context const& c) { return c.kinds_to_keys(); }),
        std::move(data), metric_value});
}

void ClientImpl::Track(std::string event_name,
                       Value data,
                       double metric_value) {
    this->TrackInternal(std::move(event_name), data, metric_value);
}

void ClientImpl::Track(std::string event_name, Value data) {
    this->TrackInternal(std::move(event_name), data, std::nullopt);
}

void ClientImpl::Track(std::string event_name) {
    this->TrackInternal(std::move(event_name), std::nullopt, std::nullopt);
}

void ClientImpl::AsyncFlush() {
    event_processor_->AsyncFlush();
}

void ClientImpl::OnDataSourceShutdown(Context context,
                                      std::function<void()> user_completion) {
    WriteContext(context);
    data_source_ = data_source_factory_();
    if (data_source_) {
        data_source_->Start();
        event_processor_->AsyncSend(events::client::IdentifyEventParams{
            std::chrono::system_clock::now(), context});
    }
    if (user_completion) {
        user_completion();
    }
}

void ClientImpl::AsyncIdentify(Context context,
                               std::function<void()> completion) {
    data_source_->AsyncShutdown(
        [this, ctx = std::move(context), cb = std::move(completion)]() {
            OnDataSourceShutdown(std::move(ctx), std::move(cb));
        });
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
        ReadContext<Context>([](Context const& c) { return c; }),
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

    if (check_type && default_value.Type() != Value::Type::kNull &&
        detail.value().Type() != default_value.Type()) {
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

flag_manager::detail::IFlagNotifier& ClientImpl::FlagNotifier() {
    return flag_updater_;
}

void ClientImpl::WaitForReadySync(std::chrono::milliseconds timeout) {
    std::unique_lock lock(init_mutex_);
    init_waiter_.wait_for(lock, timeout, [this] { return initialized_; });
}

void ClientImpl::WriteContext(Context context) {
    std::unique_lock lock(context_mutex_);
    context_ = std::move(context);
}

ClientImpl::~ClientImpl() {
    data_source_->AsyncShutdown([]() {});
    ioc_.stop();
    // TODO: Probably not the best.
    run_thread_.join();
}

}  // namespace launchdarkly::client_side::detail
