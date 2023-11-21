// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/server_side/bindings/c/config/builder.h>

#include <launchdarkly/detail/c_binding_helpers.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>

#include "../../data_systems/lazy_load/lazy_load_system.hpp"

using namespace launchdarkly::server_side;
using namespace launchdarkly::server_side::config::builders;

#define TO_BUILDER(ptr) (reinterpret_cast<ConfigBuilder*>(ptr))
#define FROM_BUILDER(ptr) (reinterpret_cast<LDServerConfigBuilder>(ptr))

#define TO_STREAM_BUILDER(ptr) \
    (reinterpret_cast<DataSystemBuilder::BackgroundSync::Streaming*>(ptr))

#define FROM_STREAM_BUILDER(ptr) \
    (reinterpret_cast<LDServerStreamingSyncBuilder>(ptr))

#define TO_POLL_BUILDER(ptr) \
    (reinterpret_cast<DataSystemBuilder::BackgroundSync::Polling*>(ptr))

#define FROM_POLL_BUILDER(ptr) \
    (reinterpret_cast<LDServerPollingSyncBuilder>(ptr))

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

LD_EXPORT(LDServerConfigBuilder)
LDServerConfigBuilder_New(char const* sdk_key) {
    LD_ASSERT_NOT_NULL(sdk_key);

    return FROM_BUILDER(new ConfigBuilder(sdk_key));
}

LD_EXPORT(LDStatus)
LDServerConfigBuilder_Build(LDServerConfigBuilder b,
                            LDServerConfig* out_config) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(out_config);

    return launchdarkly::detail::ConsumeBuilder<ConfigBuilder>(b, out_config);
}

LD_EXPORT(void)
LDServerConfigBuilder_Free(LDServerConfigBuilder builder) {
    delete TO_BUILDER(builder);
}

LD_EXPORT(void)
LDServerConfigBuilder_ServiceEndpoints_PollingBaseURL(LDServerConfigBuilder b,
                                                      char const* url) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(url);

    TO_BUILDER(b)->ServiceEndpoints().PollingBaseUrl(url);
}

LD_EXPORT(void)
LDServerConfigBuilder_ServiceEndpoints_StreamingBaseURL(LDServerConfigBuilder b,
                                                        char const* url) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(url);

    TO_BUILDER(b)->ServiceEndpoints().StreamingBaseUrl(url);
}

LD_EXPORT(void)
LDServerConfigBuilder_ServiceEndpoints_EventsBaseURL(LDServerConfigBuilder b,
                                                     char const* url) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(url);

    TO_BUILDER(b)->ServiceEndpoints().EventsBaseUrl(url);
}

LD_EXPORT(void)
LDServerConfigBuilder_ServiceEndpoints_RelayProxyBaseURL(
    LDServerConfigBuilder b,
    char const* url) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(url);

    TO_BUILDER(b)->ServiceEndpoints().RelayProxyBaseURL(url);
}

LD_EXPORT(void)
LDServerConfigBuilder_AppInfo_Identifier(LDServerConfigBuilder b,
                                         char const* app_id) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(app_id);

    TO_BUILDER(b)->AppInfo().Identifier(app_id);
}

LD_EXPORT(void)
LDServerConfigBuilder_AppInfo_Version(LDServerConfigBuilder b,
                                      char const* app_version) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(app_version);

    TO_BUILDER(b)->AppInfo().Version(app_version);
}

LD_EXPORT(void)
LDServerConfigBuilder_Offline(LDServerConfigBuilder b, bool const offline) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->Offline(offline);
}

LD_EXPORT(void)
LDServerConfigBuilder_Events_Enabled(LDServerConfigBuilder b, bool enabled) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->Events().Enabled(enabled);
}

LD_EXPORT(void)
LDServerConfigBuilder_Events_Capacity(LDServerConfigBuilder b,
                                      size_t capacity) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->Events().Capacity(capacity);
}

LD_EXPORT(void)
LDServerConfigBuilder_Events_FlushIntervalMs(LDServerConfigBuilder b,
                                             unsigned int milliseconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->Events().FlushInterval(
        std::chrono::milliseconds{milliseconds});
}

LD_EXPORT(void)
LDServerConfigBuilder_Events_AllAttributesPrivate(LDServerConfigBuilder b,
                                                  bool all_attributes_private) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->Events().AllAttributesPrivate(all_attributes_private);
}

LD_EXPORT(void)
LDServerConfigBuilder_Events_PrivateAttribute(LDServerConfigBuilder b,
                                              char const* attribute_reference) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->Events().PrivateAttribute(attribute_reference);
}

LD_EXPORT(void)
LDServerConfigBuilder_DataSystem_BackgroundSync_Streaming(
    LDServerConfigBuilder b,
    LDServerStreamingSyncBuilder stream_builder) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(stream_builder);

    BackgroundSyncBuilder::Streaming* sb = TO_STREAM_BUILDER(stream_builder);
    TO_BUILDER(b)->DataSystem().Method(
        BackgroundSyncBuilder().Synchronizer(*sb));
    LDServerStreamingSyncBuilder_Free(stream_builder);
}

LD_EXPORT(void)
LDServerConfigBuilder_DataSystem_BackgroudSync_Polling(
    LDServerConfigBuilder b,
    LDServerPollingSyncBuilder poll_builder) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(poll_builder);

    BackgroundSyncBuilder::Polling* pb = TO_POLL_BUILDER(poll_builder);
    TO_BUILDER(b)->DataSystem().Method(
        BackgroundSyncBuilder().Synchronizer(*pb));
    LDServerPollingSyncBuilder_Free(poll_builder);
}

LD_EXPORT(LDServerStreamingSyncBuilder)
LDServerStreamingSyncBuilder_New() {
    return FROM_STREAM_BUILDER(
        new DataSystemBuilder::BackgroundSync::Streaming());
}

LD_EXPORT(void)
LDServerStreamingSyncBuilder_InitialReconnectDelayMs(
    LDServerStreamingSyncBuilder b,
    unsigned int milliseconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_STREAM_BUILDER(b)->InitialReconnectDelay(
        std::chrono::milliseconds{milliseconds});
}

LD_EXPORT(void)
LDServerStreamingSyncBuilder_Free(LDServerStreamingSyncBuilder b) {
    delete TO_STREAM_BUILDER(b);
}

LD_EXPORT(LDServerPollingSyncBuilder) LDServerPollingSyncBuilder_New() {
    return FROM_POLL_BUILDER(new DataSystemBuilder::BackgroundSync::Polling());
}

LD_EXPORT(void)
LDServerPollingSyncBuilder_IntervalS(LDServerPollingSyncBuilder b,
                                     unsigned int seconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_POLL_BUILDER(b)->PollInterval(std::chrono::seconds{seconds});
}

LD_EXPORT(void)
LDServerPollingSyncBuilder_Free(LDServerPollingSyncBuilder b) {
    delete TO_POLL_BUILDER(b);
}

LD_EXPORT(void)
LDServerConfigBuilder_HttpProperties_WrapperName(LDServerConfigBuilder b,
                                                 char const* wrapper_name) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(wrapper_name);

    TO_BUILDER(b)->HttpProperties().WrapperName(wrapper_name);
}

LD_EXPORT(void)
LDServerConfigBuilder_HttpProperties_WrapperVersion(
    LDServerConfigBuilder b,
    char const* wrapper_version) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(wrapper_version);

    TO_BUILDER(b)->HttpProperties().WrapperVersion(wrapper_version);
}

LD_EXPORT(void)
LDServerConfigBuilder_HttpProperties_Header(LDServerConfigBuilder b,
                                            char const* key,
                                            char const* value) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(key);
    LD_ASSERT_NOT_NULL(value);

    TO_BUILDER(b)->HttpProperties().Header(key, value);
}

LD_EXPORT(void)
LDServerConfigBuilder_Logging_Disable(LDServerConfigBuilder b) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->Logging().Logging(LoggingBuilder::NoLogging());
}

LD_EXPORT(void)
LDServerConfigBuilder_Logging_Basic(LDServerConfigBuilder b,
                                    LDLoggingBasicBuilder basic_builder) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(basic_builder);

    LoggingBuilder::BasicLogging* bb = TO_BASIC_LOGGING_BUILDER(basic_builder);
    TO_BUILDER(b)->Logging().Logging(*bb);
    LDLoggingBasicBuilder_Free(basic_builder);
}

LD_EXPORT(void)
LDServerConfigBuilder_Logging_Custom(LDServerConfigBuilder b,
                                     LDLoggingCustomBuilder custom_builder) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(custom_builder);

    LoggingBuilder::CustomLogging* cb =
        TO_CUSTOM_LOGGING_BUILDER(custom_builder);
    TO_BUILDER(b)->Logging().Logging(*cb);
    LDLoggingCustomBuilder_Free(custom_builder);
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
