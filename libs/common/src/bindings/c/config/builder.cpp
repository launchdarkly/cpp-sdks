// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/bindings/c/config/builder.h>
#include <launchdarkly/config/client.hpp>
#include "../../../c_binding_helpers.hpp"

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

LD_EXPORT(LDClientConfigBuilder)
LDClientConfigBuilder_New(char const* sdk_key) {
    ASSERT_NOT_NULL(sdk_key);
    return FROM_BUILDER(new ConfigBuilder(sdk_key));
}

LD_EXPORT(LDStatus)
LDClientConfigBuilder_Build(LDClientConfigBuilder b,
                            LDClientConfig* out_config) {
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(out_config);
    return ConsumeBuilder<ConfigBuilder>(b, out_config);
}

LD_EXPORT(void)
LDClientConfigBuilder_Free(LDClientConfigBuilder builder) {
    if (ConfigBuilder* b = TO_BUILDER(builder)) {
        delete b;
    }
}

LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_PollingBaseURL(LDClientConfigBuilder b,
                                                      char const* url) {
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(url);
    TO_BUILDER(b)->ServiceEndpoints().PollingBaseUrl(url);
}

LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_StreamingBaseURL(LDClientConfigBuilder b,
                                                        char const* url) {
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(url);
    TO_BUILDER(b)->ServiceEndpoints().StreamingBaseUrl(url);
}

LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_EventsBaseURL(LDClientConfigBuilder b,
                                                     char const* url) {
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(url);
    TO_BUILDER(b)->ServiceEndpoints().EventsBaseUrl(url);
}

LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_RelayProxy(LDClientConfigBuilder b,
                                                  char const* url) {
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(url);
    TO_BUILDER(b)->ServiceEndpoints().RelayProxy(url);
}

LD_EXPORT(void)
LDClientConfigBuilder_AppInfo_Identifier(LDClientConfigBuilder b,
                                         char const* app_id) {
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(app_id);
    TO_BUILDER(b)->AppInfo().Identifier(app_id);
}

LD_EXPORT(void)
LDClientConfigBuilder_AppInfo_Version(LDClientConfigBuilder b,
                                      char const* app_version) {
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(app_version);
    TO_BUILDER(b)->AppInfo().Version(app_version);
}

LD_EXPORT(void)
LDClientConfigBuilder_Offline(LDClientConfigBuilder b, bool offline) {
    ASSERT_NOT_NULL(b);
    TO_BUILDER(b)->Offline(offline);
}

LD_EXPORT(void)
LDClientConfigBuilder_Events_Enabled(LDClientConfigBuilder b, bool enabled) {
    ASSERT_NOT_NULL(b);
    TO_BUILDER(b)->Events().Enabled(enabled);
}

LD_EXPORT(void)
LDClientConfigBuilder_Events_Capacity(LDClientConfigBuilder b,
                                      size_t capacity) {
    ASSERT_NOT_NULL(b);
    TO_BUILDER(b)->Events().Capacity(capacity);
}

LD_EXPORT(void)
LDClientConfigBuilder_Events_FlushIntervalMs(LDClientConfigBuilder b,
                                             unsigned int milliseconds) {
    ASSERT_NOT_NULL(b);
    TO_BUILDER(b)->Events().FlushInterval(
        std::chrono::milliseconds{milliseconds});
}

LD_EXPORT(void)
LDClientConfigBuilder_Events_AllAttributesPrivate(LDClientConfigBuilder b,
                                                  bool all_attributes_private) {
    ASSERT_NOT_NULL(b);
    TO_BUILDER(b)->Events().AllAttributesPrivate(all_attributes_private);
}

LD_EXPORT(void)
LDClientConfigBuilder_Events_PrivateAttribute(LDClientConfigBuilder b,
                                              char const* attribute_reference) {
    ASSERT_NOT_NULL(b);
    TO_BUILDER(b)->Events().PrivateAttribute(attribute_reference);
}

LD_EXPORT(void)
LDClientConfigBuilder_DataSource_WithReasons(LDClientConfigBuilder b,
                                             bool with_reasons) {
    ASSERT_NOT_NULL(b);
    TO_BUILDER(b)->DataSource().WithReasons(with_reasons);
}

LD_EXPORT(void)
LDClientConfigBuilder_DataSource_UseReport(LDClientConfigBuilder b,
                                           bool use_report) {
    ASSERT_NOT_NULL(b);
    TO_BUILDER(b)->DataSource().UseReport(use_report);
}

LD_EXPORT(void)
LDClientConfigBuilder_DataSource_MethodStream(
    LDClientConfigBuilder b,
    LDDataSourceStreamBuilder stream_builder) {
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(stream_builder);

    DataSourceBuilder::Streaming* sb = TO_STREAM_BUILDER(stream_builder);
    TO_BUILDER(b)->DataSource().Method(*sb);
    LDDataSourceStreamBuilder_Free(stream_builder);
}

LD_EXPORT(void)
LDClientConfigBuilder_DataSource_MethodPoll(
    LDClientConfigBuilder b,
    LDDataSourcePollBuilder poll_builder) {
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(poll_builder);

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
    ASSERT_NOT_NULL(b);
    TO_STREAM_BUILDER(b)->InitialReconnectDelay(
        std::chrono::milliseconds{milliseconds});
}

LD_EXPORT(void) LDDataSourceStreamBuilder_Free(LDDataSourceStreamBuilder b) {
    if (DataSourceBuilder::Streaming* builder = TO_STREAM_BUILDER(b)) {
        delete builder;
    }
}

LD_EXPORT(LDDataSourcePollBuilder) LDDataSourcePollBuilder_New() {
    return FROM_POLL_BUILDER(new DataSourceBuilder::Polling());
}

LD_EXPORT(void)
LDDataSourcePollBuilder_IntervalS(LDDataSourcePollBuilder b,
                                  unsigned int seconds) {
    ASSERT_NOT_NULL(b);
    TO_POLL_BUILDER(b)->PollInterval(std::chrono::seconds{seconds});
}

LD_EXPORT(void) LDDataSourcePollBuilder_Free(LDDataSourcePollBuilder b) {
    if (DataSourceBuilder::Polling* builder = TO_POLL_BUILDER(b)) {
        delete builder;
    }
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
