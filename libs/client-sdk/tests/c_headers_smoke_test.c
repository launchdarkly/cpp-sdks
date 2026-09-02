/*
 * Compiled as C so that C++-only syntax cannot leak into a header a C
 * consumer includes. It exercises the declarations rather than the behavior,
 * which the C++ binding tests cover.
 */

#include <launchdarkly/client_side/bindings/c/config/builder.h>
#include <launchdarkly/client_side/bindings/c/config/config.h>
#include <launchdarkly/client_side/bindings/c/config/fdv2_builder/fdv2_builder.h>
#include <launchdarkly/client_side/bindings/c/sdk.h>

void LDClientFDv2HeadersSmokeTest(void) {
    LDClientFDv2Builder fdv2 = LDClientFDv2Builder_New();
    LDClientFDv2ModeBuilder mode = LDClientFDv2ModeBuilder_New();

    LDClientFDv2Builder_InitialMode(fdv2, LD_CLIENT_CONNECTION_MODE_STREAMING);
    LDClientFDv2Builder_UsePost(fdv2, true);

    LDClientFDv2ModeBuilder_Initializer_Cache(mode);
    LDClientFDv2ModeBuilder_Initializer_Polling(
        mode, LDClientFDv2PollingBuilder_New());
    LDClientFDv2ModeBuilder_Synchronizer_Streaming(
        mode, LDClientFDv2StreamingBuilder_New());
    LDClientFDv2ModeBuilder_Synchronizer_Polling(
        mode, LDClientFDv2PollingBuilder_New());
    LDClientFDv2ModeBuilder_FallbackToFDv1(
        mode, LDClientFDv2FDv1FallbackBuilder_New());
    LDClientFDv2ModeBuilder_DisableFDv1Fallback(mode);

    LDClientFDv2Builder_CustomizeMode(fdv2, LD_CLIENT_CONNECTION_MODE_POLLING,
                                      mode);

    LDClientConfigBuilder config = LDClientConfigBuilder_New("sdk-key");
    LDClientConfigBuilder_DataSource_MethodFDv2(config, fdv2);
    LDClientConfigBuilder_Free(config);
}

void LDClientFDv2SourceBuilderFreeSmokeTest(void) {
    LDClientFDv2StreamingBuilder streaming = LDClientFDv2StreamingBuilder_New();
    LDClientFDv2PollingBuilder polling = LDClientFDv2PollingBuilder_New();
    LDClientFDv2FDv1FallbackBuilder fallback =
        LDClientFDv2FDv1FallbackBuilder_New();
    LDClientFDv2ModeBuilder mode = LDClientFDv2ModeBuilder_New();
    LDClientFDv2Builder fdv2 = LDClientFDv2Builder_New();

    LDClientFDv2StreamingBuilder_InitialReconnectDelayMs(streaming, 1000);
    LDClientFDv2StreamingBuilder_BaseURL(streaming, "https://example.com");
    LDClientFDv2PollingBuilder_IntervalS(polling, 300);
    LDClientFDv2PollingBuilder_BaseURL(polling, "https://example.com");
    LDClientFDv2FDv1FallbackBuilder_IntervalS(fallback, 300);
    LDClientFDv2FDv1FallbackBuilder_BaseURL(fallback, "https://example.com");

    LDClientFDv2StreamingBuilder_Free(streaming);
    LDClientFDv2PollingBuilder_Free(polling);
    LDClientFDv2FDv1FallbackBuilder_Free(fallback);
    LDClientFDv2ModeBuilder_Free(mode);
    LDClientFDv2Builder_Free(fdv2);
}
