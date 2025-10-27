// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/server_side/bindings/c/config/builder.h>

#include <launchdarkly/detail/c_binding_helpers.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>

#include "hook_wrapper.hpp"

using namespace launchdarkly::server_side;
using namespace launchdarkly::server_side::config::builders;

#define TO_BUILDER(ptr) (reinterpret_cast<ConfigBuilder*>(ptr))
#define FROM_BUILDER(ptr) (reinterpret_cast<LDServerConfigBuilder>(ptr))

#define TO_STREAM_BUILDER(ptr) \
    (reinterpret_cast<DataSystemBuilder::BackgroundSync::Streaming*>(ptr))

#define FROM_STREAM_BUILDER(ptr) \
    (reinterpret_cast<LDServerDataSourceStreamBuilder>(ptr))

#define TO_POLL_BUILDER(ptr) \
    (reinterpret_cast<DataSystemBuilder::BackgroundSync::Polling*>(ptr))

#define FROM_POLL_BUILDER(ptr) \
    (reinterpret_cast<LDServerDataSourcePollBuilder>(ptr))

#define TO_LAZYLOAD_BUILDER(ptr) \
    (reinterpret_cast<DataSystemBuilder::LazyLoad*>(ptr))

#define FROM_LAZYLOAD_BUILDER(ptr) \
    (reinterpret_cast<LDServerLazyLoadBuilder>(ptr))

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

#define TO_TLS_BUILDER(ptr) (reinterpret_cast<TlsBuilder*>(ptr))

#define FROM_TLS_BUILDER(ptr) \
    (reinterpret_cast<LDServerHttpPropertiesTlsBuilder>(ptr))

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
LDServerConfigBuilder_Events_ContextKeysCapacity(LDServerConfigBuilder b,
                                                 size_t context_keys_capacity) {
    LD_ASSERT_NOT_NULL(b);

    TO_BUILDER(b)->Events().ContextKeysCapacity(context_keys_capacity);
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
    LDServerDataSourceStreamBuilder stream_builder) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(stream_builder);

    BackgroundSyncBuilder::Streaming* sb = TO_STREAM_BUILDER(stream_builder);
    TO_BUILDER(b)->DataSystem().Method(
        BackgroundSyncBuilder().Synchronizer(*sb));
    LDServerDataSourceStreamBuilder_Free(stream_builder);
}

LD_EXPORT(void)
LDServerConfigBuilder_DataSystem_BackgroundSync_Polling(
    LDServerConfigBuilder b,
    LDServerDataSourcePollBuilder poll_builder) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(poll_builder);

    BackgroundSyncBuilder::Polling* pb = TO_POLL_BUILDER(poll_builder);
    TO_BUILDER(b)->DataSystem().Method(
        BackgroundSyncBuilder().Synchronizer(*pb));
    LDServerDataSourcePollBuilder_Free(poll_builder);
}

LD_EXPORT(void)
LDServerConfigBuilder_DataSystem_LazyLoad(
    LDServerConfigBuilder b,
    LDServerLazyLoadBuilder lazy_load_builder) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(lazy_load_builder);

    DataSystemBuilder::LazyLoad const* llb =
        TO_LAZYLOAD_BUILDER(lazy_load_builder);
    TO_BUILDER(b)->DataSystem().Method(*llb);
    LDServerLazyLoadBuilder_Free(lazy_load_builder);
}

LD_EXPORT(void)
LDServerConfigBuilder_DataSystem_Enabled(LDServerConfigBuilder b,
                                         bool const enabled) {
    LD_ASSERT_NOT_NULL(b);
    TO_BUILDER(b)->DataSystem().Enabled(enabled);
}

LD_EXPORT(LDServerDataSourceStreamBuilder)
LDServerDataSourceStreamBuilder_New() {
    return FROM_STREAM_BUILDER(
        new DataSystemBuilder::BackgroundSync::Streaming());
}

LD_EXPORT(void)
LDServerDataSourceStreamBuilder_InitialReconnectDelayMs(
    LDServerDataSourceStreamBuilder b,
    unsigned int milliseconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_STREAM_BUILDER(b)->InitialReconnectDelay(
        std::chrono::milliseconds{milliseconds});
}

LD_EXPORT(void)
LDServerDataSourceStreamBuilder_Filter(LDServerDataSourceStreamBuilder b,
                                       char const* filter_key) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(filter_key);

    TO_STREAM_BUILDER(b)->Filter(filter_key);
}

LD_EXPORT(void)
LDServerDataSourceStreamBuilder_Free(LDServerDataSourceStreamBuilder b) {
    delete TO_STREAM_BUILDER(b);
}

LD_EXPORT(LDServerDataSourcePollBuilder) LDServerDataSourcePollBuilder_New() {
    return FROM_POLL_BUILDER(new DataSystemBuilder::BackgroundSync::Polling());
}

LD_EXPORT(void)
LDServerDataSourcePollBuilder_IntervalS(LDServerDataSourcePollBuilder b,
                                        unsigned int seconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_POLL_BUILDER(b)->PollInterval(std::chrono::seconds{seconds});
}

LD_EXPORT(void)
LDServerDataSourcePollBuilder_Filter(LDServerDataSourcePollBuilder b,
                                     char const* filter_key) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(filter_key);

    TO_POLL_BUILDER(b)->Filter(filter_key);
}

LD_EXPORT(void)
LDServerDataSourcePollBuilder_Free(LDServerDataSourcePollBuilder b) {
    delete TO_POLL_BUILDER(b);
}

LD_EXPORT(LDServerLazyLoadBuilder)
LDServerLazyLoadBuilder_New() {
    return FROM_LAZYLOAD_BUILDER(new DataSystemBuilder::LazyLoad());
}

LD_EXPORT(void)
LDServerLazyLoadBuilder_Free(LDServerLazyLoadBuilder b) {
    delete TO_LAZYLOAD_BUILDER(b);
}

LD_EXPORT(void)
LDServerLazyLoadBuilder_SourcePtr(LDServerLazyLoadBuilder b,
                                  LDServerLazyLoadSourcePtr source) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(source);

    auto const raw_owned_ptr =
        reinterpret_cast<integrations::ISerializedDataReader*>(source);
    auto owned_ptr =
        std::unique_ptr<integrations::ISerializedDataReader>(raw_owned_ptr);
    TO_LAZYLOAD_BUILDER(b)->Source(std::move(owned_ptr));
}

LD_EXPORT(void)
LDServerLazyLoadBuilder_CacheRefreshMs(LDServerLazyLoadBuilder b,
                                       unsigned int const milliseconds) {
    LD_ASSERT_NOT_NULL(b);
    TO_LAZYLOAD_BUILDER(b)->CacheRefresh(
        std::chrono::milliseconds{milliseconds});
}

LD_EXPORT(void)
LDServerLazyLoadBuilder_CachePolicy(LDServerLazyLoadBuilder b,
                                    LDLazyLoadCacheEvictionPolicy policy) {
    LD_ASSERT_NOT_NULL(b);
    TO_LAZYLOAD_BUILDER(b)->CacheEviction(
        static_cast<DataSystemBuilder::LazyLoad::EvictionPolicy>(policy));
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
LDServerConfigBuilder_HttpProperties_Tls(
    LDServerConfigBuilder b,
    LDServerHttpPropertiesTlsBuilder tls_builder) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(tls_builder);

    TO_BUILDER(b)->HttpProperties().Tls(*TO_TLS_BUILDER(tls_builder));

    LDServerHttpPropertiesTlsBuilder_Free(tls_builder);
}

LD_EXPORT(void)
LDServerHttpPropertiesTlsBuilder_SkipVerifyPeer(
    LDServerHttpPropertiesTlsBuilder b,
    bool skip_verify_peer) {
    LD_ASSERT_NOT_NULL(b);

    TO_TLS_BUILDER(b)->SkipVerifyPeer(skip_verify_peer);
}

LD_EXPORT(void)
LDServerHttpPropertiesTlsBuilder_CustomCAFile(
    LDServerHttpPropertiesTlsBuilder b,
    char const* custom_ca_file) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(custom_ca_file);

    TO_TLS_BUILDER(b)->CustomCAFile(custom_ca_file);
}

LD_EXPORT(LDServerHttpPropertiesTlsBuilder)
LDServerHttpPropertiesTlsBuilder_New(void) {
    return FROM_TLS_BUILDER(new TlsBuilder());
}

LD_EXPORT(void)
LDServerHttpPropertiesTlsBuilder_Free(LDServerHttpPropertiesTlsBuilder b) {
    delete TO_TLS_BUILDER(b);
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

LD_EXPORT(void)
LDServerConfigBuilder_Hooks(LDServerConfigBuilder builder,
                            struct LDServerSDKHook hook) {
    LD_ASSERT_NOT_NULL(builder);
    LD_ASSERT_NOT_NULL(hook.Name);

    // Create a wrapper that bridges C callbacks to C++ Hook interface
    auto hook_wrapper = std::make_shared<bindings::CHookWrapper>(hook);

    // Register the wrapper with the config builder
    TO_BUILDER(builder)->Hooks(hook_wrapper);
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
