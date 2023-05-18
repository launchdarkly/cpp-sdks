#include <gtest/gtest.h>
#include <launchdarkly/bindings/c/config/builder.h>
#include <launchdarkly/config/client.hpp>

TEST(ClientConfigBindings, ConfigBuilderNewFree) {
    LDClientConfigBuilder builder = LDClientConfigBuilder_New("sdk-123");
    ASSERT_TRUE(builder);
    LDClientConfigBuilder_Free(builder);
}

TEST(ClientConfigBindings, ConfigBuilderEmptyResultsInError) {
    LDClientConfigBuilder builder = LDClientConfigBuilder_New("");

    LDClientConfig config = nullptr;
    LDStatus status = LDClientConfigBuilder_Build(builder, &config);

    ASSERT_FALSE(config);
    ASSERT_FALSE(LDStatus_Ok(status));
    ASSERT_TRUE(LDStatus_Error(status));

    LDStatus_Free(status);
    // LDClientConfigBuilder is consumed by Build; no need to free it.
}

TEST(ClientConfigBindings, MinimalValidConfig) {
    LDClientConfigBuilder builder = LDClientConfigBuilder_New("sdk-123");

    LDClientConfig config = nullptr;
    LDStatus status = LDClientConfigBuilder_Build(builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));
    ASSERT_TRUE(config);

    LDClientConfig_Free(config);
    // LDClientConfigBuilder is consumed by Build; no need to free it.
}

TEST(ClientConfigBindings, AllConfigs) {
    LDClientConfigBuilder builder = LDClientConfigBuilder_New("sdk-123");

    LDClientConfigBuilder_ServiceEndpoints_PollingBaseURL(
        builder, "https://launchdarkly.com");
    LDClientConfigBuilder_ServiceEndpoints_StreamingBaseURL(
        builder, "https://launchdarkly.com");
    LDClientConfigBuilder_ServiceEndpoints_EventsBaseURL(
        builder, "https://launchdarkly.com");
    LDClientConfigBuilder_ServiceEndpoints_RelayProxyBaseURL(
        builder, "https://launchdarkly.com");

    LDClientConfigBuilder_Offline(builder, false);

    LDClientConfigBuilder_AppInfo_Identifier(builder, "app");
    LDClientConfigBuilder_AppInfo_Version(builder, "v1.2.3");

    LDClientConfigBuilder_Events_Enabled(builder, true);
    LDClientConfigBuilder_Events_Capacity(builder, 100);
    LDClientConfigBuilder_Events_FlushIntervalMs(builder, 1000);
    LDClientConfigBuilder_Events_AllAttributesPrivate(builder, false);
    LDClientConfigBuilder_Events_PrivateAttribute(builder, "/foo/bar");

    LDClientConfigBuilder_DataSource_UseReport(builder, true);
    LDClientConfigBuilder_DataSource_WithReasons(builder, true);

    LDDataSourceStreamBuilder stream_builder = LDDataSourceStreamBuilder_New();
    LDDataSourceStreamBuilder_InitialReconnectDelayMs(stream_builder, 500);
    LDClientConfigBuilder_DataSource_MethodStream(builder, stream_builder);

    LDDataSourcePollBuilder poll_builder = LDDataSourcePollBuilder_New();
    LDDataSourcePollBuilder_IntervalS(poll_builder, 10);
    LDClientConfigBuilder_DataSource_MethodPoll(builder, poll_builder);

    LDClientConfigBuilder_HttpProperties_Header(builder, "foo", "bar");
    LDClientConfigBuilder_HttpProperties_WrapperName(builder, "wrapper");
    LDClientConfigBuilder_HttpProperties_WrapperVersion(builder, "v1.2.3");

    LDClientConfigBuilder_Logging_Disable(builder);

    LDLoggingBasicBuilder log_builder = LDLoggingBasicBuilder_New();
    LDLoggingBasicBuilder_Level(log_builder, LD_LOG_WARN);
    LDLoggingBasicBuilder_Tag(log_builder, "tag");

    LDClientConfigBuilder_Logging_Basic(builder, log_builder);

    LDLoggingCustomBuilder custom_logger = LDLoggingCustomBuilder_New();

    struct LDLogBackend backend;
    LDLogBackend_Init(&backend);

    LDLoggingCustomBuilder_Backend(custom_logger, backend);

    LDClientConfigBuilder_Logging_Custom(builder, custom_logger);

    LDClientConfig config = nullptr;
    LDStatus status = LDClientConfigBuilder_Build(builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));
    ASSERT_TRUE(config);

    LDClientConfig_Free(config);
}

TEST(ClientConfigBindings, CustomNoopLogger) {
    using namespace launchdarkly;

    LDClientConfigBuilder builder = LDClientConfigBuilder_New("sdk-123");

    LDLoggingCustomBuilder custom = LDLoggingCustomBuilder_New();

    struct LDLogBackend backend;
    LDLogBackend_Init(&backend);

    LDLoggingCustomBuilder_Backend(custom, backend);
    LDClientConfigBuilder_Logging_Custom(builder, custom);

    LDClientConfig config = nullptr;
    LDStatus status = LDClientConfigBuilder_Build(builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    auto client_config = reinterpret_cast<client_side::Config*>(config);

    // These should not abort since LDLogBackend_Init sets up the function
    // pointers to be no-ops.
    client_config->Logging().backend->Enabled(LogLevel::kError);
    client_config->Logging().backend->Write(LogLevel::kError, "hello");

    LDClientConfig_Free(config);
}

struct LogArgs {
    std::optional<LDLogLevel> enabled;

    struct WriteArgs {
        LDLogLevel level;
        std::string msg;
        WriteArgs(LDLogLevel level, char const* msg) : level(level), msg(msg) {}
    };

    std::optional<WriteArgs> write;
};

TEST(ClientConfigBindings, CustomLogger) {
    using namespace launchdarkly;

    LDClientConfigBuilder builder = LDClientConfigBuilder_New("sdk-123");

    LDLoggingCustomBuilder custom = LDLoggingCustomBuilder_New();

    struct LDLogBackend backend;
    LDLogBackend_Init(&backend);

    LogArgs args;

    backend.UserData = &args;

    backend.Enabled = [](enum LDLogLevel level, void* user_data) -> bool {
        auto spy = reinterpret_cast<decltype(args)*>(user_data);
        spy->enabled.emplace(level);
        return true;
    };
    backend.Write = [](enum LDLogLevel level, char const* msg,
                       void* user_data) {
        auto spy = reinterpret_cast<decltype(args)*>(user_data);
        spy->write.emplace(level, msg);
    };

    LDLoggingCustomBuilder_Backend(custom, backend);
    LDClientConfigBuilder_Logging_Custom(builder, custom);

    LDClientConfig config = nullptr;
    LDStatus status = LDClientConfigBuilder_Build(builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    auto client_config = reinterpret_cast<client_side::Config*>(config);

    client_config->Logging().backend->Enabled(LogLevel::kWarn);
    client_config->Logging().backend->Write(LogLevel::kError, "hello");

    LDClientConfig_Free(config);

    ASSERT_TRUE(args.enabled);
    ASSERT_EQ(args.enabled, LD_LOG_WARN);

    ASSERT_TRUE(args.write);
    ASSERT_EQ(args.write->level, LD_LOG_ERROR);
    ASSERT_EQ(args.write->msg, "hello");
}
