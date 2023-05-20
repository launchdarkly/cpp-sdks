// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/bindings/c/config/builder.h>
#include <launchdarkly/config/client.hpp>
#include <launchdarkly/detail/c_binding_helpers.hpp>
#include <launchdarkly/persistence/persistence.hpp>

using namespace launchdarkly::client_side;

#define TO_BUILDER(ptr) (reinterpret_cast<ConfigBuilder*>(ptr))
#define FROM_BUILDER(ptr) (reinterpret_cast<LDClientConfigBuilder>(ptr))

#define TO_STREAM_BUILDER(ptr) \
    (reinterpret_cast<DataSourceBuilder::Streaming*>(ptr))

#define FROM_STREAM_BUILDER(ptr) \
    (reinterpret_cast<LDDataSourceStreamBuilder>(ptr))

#define TO_POLL_BUILDER(ptr) \
    (reinterpret_cast<DataSourceBuilder::Polling*>(ptr))

#define FROM_POLL_BUILDER(ptr) (reinterpret_cast<LDDataSourcePollBuilder>(ptr))

#define TO_BASIC_LOGGING_BUILDER(ptr) \
    (reinterpret_cast<LoggingBuilder::BasicLogging*>(ptr))

#define FROM_BASIC_LOGGING_BUILDER(ptr) \
    (reinterpret_cast<LDLoggingBasicBuilder>(ptr))

#define TO_CUSTOM_LOGGING_BUILDER(ptr) \
    (reinterpret_cast<LoggingBuilder::CustomLogging*>(ptr))

#define FROM_CUSTOM_LOGGING_BUILDER(ptr) \
    (reinterpret_cast<LDLoggingCustomBuilder>(ptr))

#define TO_CUSTOM_PERSISTENCE_BUILDER(ptr) \
    (reinterpret_cast<PersistenceBuilder::CustomBuilder*>(ptr))

#define FROM_CUSTOM_PERSISTENCE_BUILDER(ptr) \
    (reinterpret_cast<LDPersistenceCustomBuilder>(ptr))

/**
 * Utility class to allow user-provided backends to satisfy the ILogBackend
 * interface.
 */
class LogBackendWrapper : public launchdarkly::ILogBackend {
   public:
    explicit LogBackendWrapper(LDLogBackend backend) : backend_(backend) {}
    bool Enabled(launchdarkly::LogLevel level) noexcept override {
        return backend_.Enabled(static_cast<LDLogLevel>(level),
                                backend_.UserData);
    }
    void Write(launchdarkly::LogLevel level,
               std::string message) noexcept override {
        return backend_.Write(static_cast<LDLogLevel>(level), message.c_str(),
                              backend_.UserData);
    }

   private:
    LDLogBackend backend_;
};

class PersistenceImplementationWrapper : public IPersistence {
   public:
    explicit PersistenceImplementationWrapper(LDPersistence impl)
        : impl_(impl) {}

    void Set(std::string storage_namespace,
             std::string key,
             std::string data) noexcept override {
        return impl_.Set(storage_namespace.c_str(), key.c_str(), data.c_str(),
                         impl_.UserData);
    }

    void Remove(std::string storage_namespace,
                std::string key) noexcept override {
        return impl_.Remove(storage_namespace.c_str(), key.c_str(),
                            impl_.UserData);
    }

    std::optional<std::string> Read(std::string storage_namespace,
                                    std::string key) noexcept override {
        char const* read_value;
        std::optional<std::string> value_as_optional_string;
        auto size = impl_.Read(storage_namespace.c_str(), key.c_str(),
                               &read_value, impl_.UserData);
        if (size && read_value) {
            // Get a copy as a string.
            value_as_optional_string = std::string(read_value, size);
        }

        if (read_value) {
            impl_.FreeRead(read_value, impl_.UserData);
        }
        return value_as_optional_string;
    }

   private:
    LDPersistence impl_;
};

LD_EXPORT(LDClientConfigBuilder)
LDClientConfigBuilder_New(char const* sdk_key) {
    LD_ASSERT_NOT_NULL(sdk_key);

    return FROM_BUILDER(new ConfigBuilder(sdk_key));
}

LD_EXPORT(LDStatus)
LDClientConfigBuilder_Build(LDClientConfigBuilder b,
                            LDClientConfig* out_config) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(out_config);

    return launchdarkly::ConsumeBuilder<ConfigBuilder>(b, out_config);
}

LD_EXPORT(void)
LDClientConfigBuilder_Free(LDClientConfigBuilder builder) {
    delete TO_BUILDER(builder);
}

LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_PollingBaseURL(LDClientConfigBuilder b,
                                                      char const* url) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(url);

    TO_BUILDER(b)->ServiceEndpoints().PollingBaseUrl(url);
}

LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_StreamingBaseURL(LDClientConfigBuilder b,
                                                        char const* url) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(url);

    TO_BUILDER(b)->ServiceEndpoints().StreamingBaseUrl(url);
}

LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_EventsBaseURL(LDClientConfigBuilder b,
                                                     char const* url) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(url);

    TO_BUILDER(b)->ServiceEndpoints().EventsBaseUrl(url);
}

LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_RelayProxyBaseURL(
    LDClientConfigBuilder b,
    char const* url) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(url);

    TO_BUILDER(b)->ServiceEndpoints().RelayProxyBaseURL(url);
}

LD_EXPORT(void)
LDClientConfigBuilder_AppInfo_Identifier(LDClientConfigBuilder b,
                                         char const* app_id) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(app_id);

    TO_BUILDER(b)->AppInfo().Identifier(app_id);
}

LD_EXPORT(void)
LDClientConfigBuilder_AppInfo_Version(LDClientConfigBuilder b,
                                      char const* app_version) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(app_version);

    TO_BUILDER(b)->AppInfo().Version(app_version);
}

LD_EXPORT(void)
LDClientConfigBuilder_Offline(LDClientConfigBuilder b, bool offline) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->Offline(offline);
}

LD_EXPORT(void)
LDClientConfigBuilder_Events_Enabled(LDClientConfigBuilder b, bool enabled) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->Events().Enabled(enabled);
}

LD_EXPORT(void)
LDClientConfigBuilder_Events_Capacity(LDClientConfigBuilder b,
                                      size_t capacity) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->Events().Capacity(capacity);
}

LD_EXPORT(void)
LDClientConfigBuilder_Events_FlushIntervalMs(LDClientConfigBuilder b,
                                             unsigned int milliseconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->Events().FlushInterval(
        std::chrono::milliseconds{milliseconds});
}

LD_EXPORT(void)
LDClientConfigBuilder_Events_AllAttributesPrivate(LDClientConfigBuilder b,
                                                  bool all_attributes_private) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->Events().AllAttributesPrivate(all_attributes_private);
}

LD_EXPORT(void)
LDClientConfigBuilder_Events_PrivateAttribute(LDClientConfigBuilder b,
                                              char const* attribute_reference) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->Events().PrivateAttribute(attribute_reference);
}

LD_EXPORT(void)
LDClientConfigBuilder_DataSource_WithReasons(LDClientConfigBuilder b,
                                             bool with_reasons) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->DataSource().WithReasons(with_reasons);
}

LD_EXPORT(void)
LDClientConfigBuilder_DataSource_UseReport(LDClientConfigBuilder b,
                                           bool use_report) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->DataSource().UseReport(use_report);
}

LD_EXPORT(void)
LDClientConfigBuilder_DataSource_MethodStream(
    LDClientConfigBuilder b,
    LDDataSourceStreamBuilder stream_builder) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(stream_builder);

    DataSourceBuilder::Streaming* sb = TO_STREAM_BUILDER(stream_builder);
    TO_BUILDER(b)->DataSource().Method(*sb);
    LDDataSourceStreamBuilder_Free(stream_builder);
}

LD_EXPORT(void)
LDClientConfigBuilder_DataSource_MethodPoll(
    LDClientConfigBuilder b,
    LDDataSourcePollBuilder poll_builder) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(poll_builder);

    DataSourceBuilder::Polling* pb = TO_POLL_BUILDER(poll_builder);
    TO_BUILDER(b)->DataSource().Method(*pb);
    LDDataSourcePollBuilder_Free(poll_builder);
}

LD_EXPORT(LDDataSourceStreamBuilder) LDDataSourceStreamBuilder_New() {
    return FROM_STREAM_BUILDER(new DataSourceBuilder::Streaming());
}

LD_EXPORT(void)
LDDataSourceStreamBuilder_InitialReconnectDelayMs(LDDataSourceStreamBuilder b,
                                                  unsigned int milliseconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_STREAM_BUILDER(b)->InitialReconnectDelay(
        std::chrono::milliseconds{milliseconds});
}

LD_EXPORT(void) LDDataSourceStreamBuilder_Free(LDDataSourceStreamBuilder b) {
    delete TO_STREAM_BUILDER(b);
}

LD_EXPORT(LDDataSourcePollBuilder) LDDataSourcePollBuilder_New() {
    return FROM_POLL_BUILDER(new DataSourceBuilder::Polling());
}

LD_EXPORT(void)
LDDataSourcePollBuilder_IntervalS(LDDataSourcePollBuilder b,
                                  unsigned int seconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_POLL_BUILDER(b)->PollInterval(std::chrono::seconds{seconds});
}

LD_EXPORT(void) LDDataSourcePollBuilder_Free(LDDataSourcePollBuilder b) {
    delete TO_POLL_BUILDER(b);
}

LD_EXPORT(void)
LDClientConfigBuilder_HttpProperties_WrapperName(LDClientConfigBuilder b,
                                                 char const* wrapper_name) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(wrapper_name);

    TO_BUILDER(b)->HttpProperties().WrapperName(wrapper_name);
}

LD_EXPORT(void)
LDClientConfigBuilder_HttpProperties_WrapperVersion(
    LDClientConfigBuilder b,
    char const* wrapper_version) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(wrapper_version);

    TO_BUILDER(b)->HttpProperties().WrapperVersion(wrapper_version);
}

LD_EXPORT(void)
LDClientConfigBuilder_HttpProperties_Header(LDClientConfigBuilder b,
                                            char const* key,
                                            char const* value) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(key);
    LD_ASSERT_NOT_NULL(value);

    TO_BUILDER(b)->HttpProperties().Header(key, value);
}

LD_EXPORT(LDLoggingBasicBuilder) LDLoggingBasicBuilder_New() {
    return FROM_BASIC_LOGGING_BUILDER(new LoggingBuilder::BasicLogging());
}

LD_EXPORT(void) LDLoggingBasicBuilder_Free(LDLoggingBasicBuilder b) {
    delete TO_BASIC_LOGGING_BUILDER(b);
}

LD_EXPORT(void)
LDClientConfigBuilder_Logging_Disable(LDClientConfigBuilder b) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->Logging().Logging(LoggingBuilder::NoLogging());
}

LD_EXPORT(void)
LDLoggingBasicBuilder_Level(LDLoggingBasicBuilder b, enum LDLogLevel level) {
    using launchdarkly::LogLevel;
    LD_ASSERT_NOT_NULL(b);

    LoggingBuilder::BasicLogging* logger = TO_BASIC_LOGGING_BUILDER(b);
    logger->Level(static_cast<LogLevel>(level));
}

void LDLoggingBasicBuilder_Tag(LDLoggingBasicBuilder b, char const* tag) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(tag);

    TO_BASIC_LOGGING_BUILDER(b)->Tag(tag);
}

LD_EXPORT(void)
LDClientConfigBuilder_Logging_Basic(LDClientConfigBuilder b,
                                    LDLoggingBasicBuilder basic_builder) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(basic_builder);

    LoggingBuilder::BasicLogging* bb = TO_BASIC_LOGGING_BUILDER(basic_builder);
    TO_BUILDER(b)->Logging().Logging(*bb);
    LDLoggingBasicBuilder_Free(basic_builder);
}

LD_EXPORT(void) LDLogBackend_Init(struct LDLogBackend* backend) {
    backend->Enabled = [](enum LDLogLevel, void*) { return false; };
    backend->Write = [](enum LDLogLevel, char const*, void*) {};
    backend->UserData = nullptr;
}

LD_EXPORT(void) LDPersistence_Init(struct LDPersistence* impl) {
    impl->Set = [](char const* storage_namespace, char const* key,
                   char const* data, void* user_data) {};
    impl->Remove = [](char const* storage_namespace, char const* key,
                      void* user_data) {};
    impl->Read = [](char const* storage_namespace, char const* key,
                    char const** read_value, void* user_data) {
        *read_value = nullptr;
        return (size_t)0;
    };
    impl->FreeRead = [](char const* value, void* user_data) {};
    impl->UserData = nullptr;
}

LD_EXPORT(LDLoggingCustomBuilder) LDLoggingCustomBuilder_New() {
    return FROM_CUSTOM_LOGGING_BUILDER(new LoggingBuilder::CustomLogging());
}

LD_EXPORT(void) LDLoggingCustomBuilder_Free(LDLoggingCustomBuilder b) {
    delete TO_CUSTOM_LOGGING_BUILDER(b);
}

LD_EXPORT(void)
LDClientConfigBuilder_Logging_Custom(LDClientConfigBuilder b,
                                     LDLoggingCustomBuilder custom_builder) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(custom_builder);

    LoggingBuilder::CustomLogging* cb =
        TO_CUSTOM_LOGGING_BUILDER(custom_builder);
    TO_BUILDER(b)->Logging().Logging(*cb);
    LDLoggingCustomBuilder_Free(custom_builder);
}

LD_EXPORT(void)
LDLoggingCustomBuilder_Backend(LDLoggingCustomBuilder b, LDLogBackend backend) {
    LD_ASSERT_NOT_NULL(b);

    TO_CUSTOM_LOGGING_BUILDER(b)->Backend(
        std::make_shared<LogBackendWrapper>(backend));
}

LD_EXPORT(LDPersistenceCustomBuilder) LDPersistenceCustomBuilder_New() {
    return FROM_CUSTOM_PERSISTENCE_BUILDER(
        new PersistenceBuilder::CustomBuilder());
}

LD_EXPORT(void) LDPersistenceCustomBuilder_Free(LDPersistenceCustomBuilder b) {
    delete TO_CUSTOM_PERSISTENCE_BUILDER(b);
}

LD_EXPORT(void)
LDPersistenceCustomBuilder_Implementation(LDPersistenceCustomBuilder b,
                                          LDPersistence impl) {
    LD_ASSERT_NOT_NULL(b);

    TO_CUSTOM_PERSISTENCE_BUILDER(b)->Implementation(
        std::make_shared<PersistenceImplementationWrapper>(impl));
}

LD_EXPORT(void)
LDClientConfigBuilder_Persistence_Custom(
    LDClientConfigBuilder b,
    LDPersistenceCustomBuilder custom_builder) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(custom_builder);

    PersistenceBuilder::CustomBuilder* cb =
        TO_CUSTOM_PERSISTENCE_BUILDER(custom_builder);
    TO_BUILDER(b)->Persistence().Type(std::move(*cb));
    LDPersistenceCustomBuilder_Free(custom_builder);
}

LD_EXPORT(void)
LDClientConfigBuilder_Persistence_None(LDClientConfigBuilder b) {}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
