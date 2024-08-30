#include <launchdarkly/config/shared/builders/data_source_builder.hpp>

namespace launchdarkly::config::shared::builders {
template <typename SDK>
struct MethodVisitor {
};

template <>
struct MethodVisitor<ClientSDK> {
    using SDK = ClientSDK;
    using Result =
    tl::expected<std::variant<built::StreamingConfig<SDK>, built::PollingConfig<
                                  SDK>>, Error>;

    Result operator()(StreamingBuilder<SDK> const& streaming) const {
        if (auto cfg_or_error = streaming.Build(); !cfg_or_error) {
            return tl::make_unexpected(cfg_or_error.error());
        } else {
            return *cfg_or_error;
        }
    }

    Result operator()(PollingBuilder<SDK> const& polling) const {
        if (auto cfg_or_error = polling.Build(); !cfg_or_error) {
            return tl::make_unexpected(cfg_or_error.error());
        } else {
            return *cfg_or_error;
        }
    }
};

template <typename SDK>
StreamingBuilder<SDK>::StreamingBuilder()
    : config_(Defaults<SDK>::StreamingConfig()) {
}

template <typename SDK>
StreamingBuilder<SDK>& StreamingBuilder<SDK>::InitialReconnectDelay(
    std::chrono::milliseconds initial_reconnect_delay) {
    config_.initial_reconnect_delay = initial_reconnect_delay;
    return *this;
}

template <typename SDK>
StreamingBuilder<SDK>& StreamingBuilder<SDK>::Filter(std::string filter_key) {
    config_.filter_key = std::move(filter_key);
    return *this;
}

template <typename SDK>
tl::expected<built::StreamingConfig<SDK>, Error> StreamingBuilder<
    SDK>::Build() const {
    if (config_.filter_key.empty()) {
        return tl::make_unexpected(Error::kConfig_DataSource_EmptyFilterKey);
    }
    return config_;
}

template <typename SDK>
PollingBuilder<SDK>::PollingBuilder()
    : config_(Defaults<SDK>::PollingConfig()) {
}

template <typename SDK>
PollingBuilder<SDK>& PollingBuilder<SDK>::PollInterval(
    std::chrono::seconds poll_interval) {
    config_.poll_interval = poll_interval;
    return *this;
}

template <typename SDK>
PollingBuilder<SDK>& PollingBuilder<SDK>::Filter(std::string filter_key) {
    config_.filter_key = std::move(filter_key);
    return *this;
}


template <typename SDK>
tl::expected<built::PollingConfig<SDK>, Error> PollingBuilder<
    SDK>::Build() const {
    if (config_.filter_key.empty()) {
        return tl::make_unexpected(Error::kConfig_DataSource_EmptyFilterKey);
    }
    return config_;
}

DataSourceBuilder<ClientSDK>::DataSourceBuilder()
    : with_reasons_(false), use_report_(false), method_(Streaming()) {
}

DataSourceBuilder<ClientSDK>& DataSourceBuilder<ClientSDK>::WithReasons(
    bool value) {
    with_reasons_ = value;
    return *this;
}

DataSourceBuilder<ClientSDK>& DataSourceBuilder<ClientSDK>::UseReport(
    bool value) {
    use_report_ = value;
    return *this;
}

DataSourceBuilder<ClientSDK>& DataSourceBuilder<ClientSDK>::Method(
    StreamingBuilder<ClientSDK> builder) {
    method_ = std::move(builder);
    return *this;
}

DataSourceBuilder<ClientSDK>& DataSourceBuilder<ClientSDK>::Method(
    PollingBuilder<ClientSDK> builder) {
    method_ = std::move(builder);
    return *this;
}

tl::expected<built::DataSourceConfig<ClientSDK>, Error> DataSourceBuilder<
    ClientSDK>::Build() const {
    auto ds_config = std::visit(MethodVisitor<ClientSDK>(), method_);
    if (!ds_config) {
        return tl::make_unexpected(ds_config.error());
    }
    return built::DataSourceConfig<ClientSDK>{*ds_config, with_reasons_,
                                              use_report_};
}

template class PollingBuilder<ClientSDK>;
template class PollingBuilder<ServerSDK>;

template class StreamingBuilder<ClientSDK>;
template class StreamingBuilder<ServerSDK>;
} // namespace launchdarkly::config::shared::builders
