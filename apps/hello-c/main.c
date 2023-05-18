#include <launchdarkly/client_side/bindings/c/sdk.h>

#include <launchdarkly/bindings/c/config/builder.h>
#include <launchdarkly/bindings/c/context_builder.h>

#include <stdio.h>
#include <stdlib.h>
int main() {
    char const* key = getenv("STG_SDK_KEY");
    if (!key) {
        printf("Set environment variable STG_SDK_KEY to the sdk key\n");
        return 1;
    }

    // BEGIN CONFIG

    LDClientConfigBuilder builder = LDClientConfigBuilder_New("sdk-123");

    LDClientConfigBuilder_ServiceEndpoints_PollingBaseURL(
        builder, "http://sdk.launchdarkly.com");
    LDClientConfigBuilder_ServiceEndpoints_StreamingBaseURL(
        builder, "https://stream.launchdarkly.com");
    LDClientConfigBuilder_ServiceEndpoints_EventsBaseURL(
        builder, "https://events.launchdarkly.com");

    LDClientConfigBuilder_Events_FlushIntervalMs(builder, 5000);

    LDClientConfigBuilder_DataSource_UseReport(builder, true);
    LDClientConfigBuilder_DataSource_WithReasons(builder, true);

    LDDataSourcePollBuilder poll_builder = LDDataSourcePollBuilder_New();
    LDDataSourcePollBuilder_IntervalS(poll_builder, 30);
    LDClientConfigBuilder_DataSource_MethodPoll(builder, poll_builder);

    LDClientConfig config;
    LDStatus status = LDClientConfigBuilder_Build(builder, &config);

    if (!LDStatus_Ok(status)) {
        printf("%s\n", LDStatus_Error(status));
        return 1;
    }

    LDContextBuilder context_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(context_builder, "user", "ryan");

    LDContext context = LDContextBuilder_Build(context_builder);

    LDClientSDK client = LDClientSDK_New(config, context);
    //
    //    std::cout << "Initial Status: " << client.DataSourceStatus().Status()
    //              << std::endl;
    //
    //    client.DataSourceStatus().OnDataSourceStatusChange([](auto status) {
    //        std::cout << "Got status: " << status << std::endl;
    //    });
    //
    //    client.FlagNotifier().OnFlagChange("my-boolean-flag", [](auto event) {
    //        std::cout << "Got flag change: " << *event << std::endl;
    //    });
    //
    //    client.WaitForReadySync(std::chrono::seconds(30));

    LDEvalDetail detail;
    if (LDClientSDK_BoolVariationDetail(client, "my-boolean-flag", false,
                                        &detail)) {
        printf("Value was: true\n");
    } else {
        printf("Value was: false\n");
    }

    LDEvalReason reason;
    if (LDEvalDetail_Reason(detail, &reason)) {
        printf("Reason was: %d\n", LDEvalReason_Kind(reason));
    }

    // Sit around.
    printf("Press enter to exit\n\n");
    getchar();
}
