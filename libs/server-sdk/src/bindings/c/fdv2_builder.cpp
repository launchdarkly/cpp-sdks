// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/server_side/bindings/c/config/builder.h>
#include <launchdarkly/server_side/bindings/c/config/fdv2_builder/fdv2_builder.h>

#include <launchdarkly/detail/c_binding_helpers.hpp>
#include <launchdarkly/server_side/config/builders/data_system/background_sync_builder.hpp>
#include <launchdarkly/server_side/config/builders/data_system/fdv2_builder.hpp>

#include <chrono>

using namespace launchdarkly::server_side::config::builders;

#define TO_FDV2_BUILDER(ptr) (reinterpret_cast<FDv2Builder*>(ptr))
#define FROM_FDV2_BUILDER(ptr) (reinterpret_cast<LDServerFDv2Builder>(ptr))

#define TO_FDV2_STREAM_BUILDER(ptr) \
    (reinterpret_cast<FDv2Builder::Streaming*>(ptr))
#define FROM_FDV2_STREAM_BUILDER(ptr) \
    (reinterpret_cast<LDServerFDv2StreamingBuilder>(ptr))

#define TO_FDV2_POLL_BUILDER(ptr) (reinterpret_cast<FDv2Builder::Polling*>(ptr))
#define FROM_FDV2_POLL_BUILDER(ptr) \
    (reinterpret_cast<LDServerFDv2PollingBuilder>(ptr))

/* LDServerDataSourceStreamBuilder / LDServerDataSourcePollBuilder wrap
 * BackgroundSyncBuilder::Streaming / Polling, which are aliases for the same
 * StreamingBuilder<ServerSDK> / PollingBuilder<ServerSDK> that FDv2Builder's
 * FDv1Fallback overloads accept. */
#define TO_FDV1_STREAM_BUILDER(ptr) \
    (reinterpret_cast<BackgroundSyncBuilder::Streaming*>(ptr))
#define TO_FDV1_POLL_BUILDER(ptr) \
    (reinterpret_cast<BackgroundSyncBuilder::Polling*>(ptr))

LD_EXPORT(LDServerFDv2Builder)
LDServerFDv2Builder_Default(void) {
    return FROM_FDV2_BUILDER(new FDv2Builder(FDv2Builder::Default()));
}

LD_EXPORT(LDServerFDv2Builder)
LDServerFDv2Builder_Custom(void) {
    return FROM_FDV2_BUILDER(new FDv2Builder(FDv2Builder::Custom()));
}

LD_EXPORT(void)
LDServerFDv2Builder_Free(LDServerFDv2Builder b) {
    delete TO_FDV2_BUILDER(b);
}

LD_EXPORT(LDServerFDv2StreamingBuilder)
LDServerFDv2StreamingBuilder_New(void) {
    return FROM_FDV2_STREAM_BUILDER(new FDv2Builder::Streaming());
}

LD_EXPORT(void)
LDServerFDv2StreamingBuilder_InitialReconnectDelayMs(
    LDServerFDv2StreamingBuilder b,
    unsigned int milliseconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_FDV2_STREAM_BUILDER(b)->InitialReconnectDelay(
        std::chrono::milliseconds{milliseconds});
}

LD_EXPORT(void)
LDServerFDv2StreamingBuilder_BaseUrl(LDServerFDv2StreamingBuilder b,
                                     char const* base_url) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(base_url);

    TO_FDV2_STREAM_BUILDER(b)->BaseUrl(base_url);
}

LD_EXPORT(void)
LDServerFDv2StreamingBuilder_Free(LDServerFDv2StreamingBuilder b) {
    delete TO_FDV2_STREAM_BUILDER(b);
}

LD_EXPORT(LDServerFDv2PollingBuilder)
LDServerFDv2PollingBuilder_New(void) {
    return FROM_FDV2_POLL_BUILDER(new FDv2Builder::Polling());
}

LD_EXPORT(void)
LDServerFDv2PollingBuilder_PollIntervalS(LDServerFDv2PollingBuilder b,
                                         unsigned int seconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_FDV2_POLL_BUILDER(b)->PollInterval(std::chrono::seconds{seconds});
}

LD_EXPORT(void)
LDServerFDv2PollingBuilder_BaseUrl(LDServerFDv2PollingBuilder b,
                                   char const* base_url) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(base_url);

    TO_FDV2_POLL_BUILDER(b)->BaseUrl(base_url);
}

LD_EXPORT(void)
LDServerFDv2PollingBuilder_Free(LDServerFDv2PollingBuilder b) {
    delete TO_FDV2_POLL_BUILDER(b);
}

LD_EXPORT(void)
LDServerFDv2Builder_Initializer_Polling(LDServerFDv2Builder b,
                                        LDServerFDv2PollingBuilder polling) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(polling);

    TO_FDV2_BUILDER(b)->Initializer(*TO_FDV2_POLL_BUILDER(polling));
    LDServerFDv2PollingBuilder_Free(polling);
}

LD_EXPORT(void)
LDServerFDv2Builder_Synchronizer_Streaming(
    LDServerFDv2Builder b,
    LDServerFDv2StreamingBuilder streaming) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(streaming);

    TO_FDV2_BUILDER(b)->Synchronizer(*TO_FDV2_STREAM_BUILDER(streaming));
    LDServerFDv2StreamingBuilder_Free(streaming);
}

LD_EXPORT(void)
LDServerFDv2Builder_Synchronizer_Polling(LDServerFDv2Builder b,
                                         LDServerFDv2PollingBuilder polling) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(polling);

    TO_FDV2_BUILDER(b)->Synchronizer(*TO_FDV2_POLL_BUILDER(polling));
    LDServerFDv2PollingBuilder_Free(polling);
}

LD_EXPORT(void)
LDServerFDv2Builder_FDv1Fallback_Streaming(
    LDServerFDv2Builder b,
    LDServerDataSourceStreamBuilder fdv1_streaming) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(fdv1_streaming);

    TO_FDV2_BUILDER(b)->FDv1Fallback(*TO_FDV1_STREAM_BUILDER(fdv1_streaming));
    LDServerDataSourceStreamBuilder_Free(fdv1_streaming);
}

LD_EXPORT(void)
LDServerFDv2Builder_FDv1Fallback_Polling(
    LDServerFDv2Builder b,
    LDServerDataSourcePollBuilder fdv1_polling) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(fdv1_polling);

    TO_FDV2_BUILDER(b)->FDv1Fallback(*TO_FDV1_POLL_BUILDER(fdv1_polling));
    LDServerDataSourcePollBuilder_Free(fdv1_polling);
}

LD_EXPORT(void)
LDServerFDv2Builder_DisableFDv1Fallback(LDServerFDv2Builder b) {
    LD_ASSERT_NOT_NULL(b);

    TO_FDV2_BUILDER(b)->DisableFDv1Fallback();
}

LD_EXPORT(void)
LDServerFDv2Builder_FallbackTimeoutMs(LDServerFDv2Builder b,
                                      unsigned int milliseconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_FDV2_BUILDER(b)->FallbackTimeout(
        std::chrono::milliseconds{milliseconds});
}

LD_EXPORT(void)
LDServerFDv2Builder_RecoveryTimeoutMs(LDServerFDv2Builder b,
                                      unsigned int milliseconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_FDV2_BUILDER(b)->RecoveryTimeout(
        std::chrono::milliseconds{milliseconds});
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
