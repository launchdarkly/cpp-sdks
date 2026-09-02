// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/client_side/bindings/c/config/builder.h>
#include <launchdarkly/client_side/bindings/c/config/fdv2_builder/fdv2_builder.h>

#include <launchdarkly/config/client.hpp>
#include <launchdarkly/detail/c_binding_helpers.hpp>

#include <chrono>
#include <utility>

using namespace launchdarkly::client_side;

#define TO_FDV2_BUILDER(ptr) (reinterpret_cast<FDv2Builder*>(ptr))
#define FROM_FDV2_BUILDER(ptr) (reinterpret_cast<LDClientFDv2Builder>(ptr))

#define TO_MODE_BUILDER(ptr) (reinterpret_cast<FDv2Builder::Mode*>(ptr))
#define FROM_MODE_BUILDER(ptr) (reinterpret_cast<LDClientFDv2ModeBuilder>(ptr))

#define TO_STREAM_BUILDER(ptr) (reinterpret_cast<FDv2Builder::Streaming*>(ptr))
#define FROM_STREAM_BUILDER(ptr) \
    (reinterpret_cast<LDClientFDv2StreamingBuilder>(ptr))

#define TO_POLL_BUILDER(ptr) (reinterpret_cast<FDv2Builder::Polling*>(ptr))
#define FROM_POLL_BUILDER(ptr) \
    (reinterpret_cast<LDClientFDv2PollingBuilder>(ptr))

#define TO_FALLBACK_BUILDER(ptr) \
    (reinterpret_cast<FDv2Builder::FDv1Fallback*>(ptr))
#define FROM_FALLBACK_BUILDER(ptr) \
    (reinterpret_cast<LDClientFDv2FDv1FallbackBuilder>(ptr))

namespace {

ConnectionMode ToConnectionMode(enum LDClientConnectionMode mode) {
    switch (mode) {
        case LD_CLIENT_CONNECTION_MODE_POLLING:
            return ConnectionMode::kPolling;
        case LD_CLIENT_CONNECTION_MODE_OFFLINE:
            return ConnectionMode::kOffline;
        case LD_CLIENT_CONNECTION_MODE_STREAMING:
        default:
            return ConnectionMode::kStreaming;
    }
}

}  // namespace

LD_EXPORT(LDClientFDv2Builder)
LDClientFDv2Builder_New(void) {
    return FROM_FDV2_BUILDER(new FDv2Builder());
}

LD_EXPORT(void)
LDClientFDv2Builder_Free(LDClientFDv2Builder b) {
    delete TO_FDV2_BUILDER(b);
}

LD_EXPORT(void)
LDClientFDv2Builder_InitialMode(LDClientFDv2Builder b,
                                enum LDClientConnectionMode mode) {
    LD_ASSERT_NOT_NULL(b);

    TO_FDV2_BUILDER(b)->InitialMode(ToConnectionMode(mode));
}

LD_EXPORT(void)
LDClientFDv2Builder_UsePost(LDClientFDv2Builder b, bool use_post) {
    LD_ASSERT_NOT_NULL(b);

    TO_FDV2_BUILDER(b)->UsePost(use_post);
}

LD_EXPORT(void)
LDClientFDv2Builder_CustomizeMode(LDClientFDv2Builder b,
                                  enum LDClientConnectionMode mode,
                                  LDClientFDv2ModeBuilder mode_builder) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(mode_builder);

    TO_FDV2_BUILDER(b)->CustomizeMode(ToConnectionMode(mode),
                                      *TO_MODE_BUILDER(mode_builder));
    LDClientFDv2ModeBuilder_Free(mode_builder);
}

LD_EXPORT(LDClientFDv2ModeBuilder)
LDClientFDv2ModeBuilder_New(void) {
    return FROM_MODE_BUILDER(new FDv2Builder::Mode());
}

LD_EXPORT(void)
LDClientFDv2ModeBuilder_Free(LDClientFDv2ModeBuilder b) {
    delete TO_MODE_BUILDER(b);
}

LD_EXPORT(void)
LDClientFDv2ModeBuilder_Initializer_Cache(LDClientFDv2ModeBuilder b) {
    LD_ASSERT_NOT_NULL(b);

    TO_MODE_BUILDER(b)->Initializer(FDv2Builder::Cache());
}

LD_EXPORT(void)
LDClientFDv2ModeBuilder_Initializer_Polling(
    LDClientFDv2ModeBuilder b,
    LDClientFDv2PollingBuilder polling) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(polling);

    TO_MODE_BUILDER(b)->Initializer(*TO_POLL_BUILDER(polling));
    LDClientFDv2PollingBuilder_Free(polling);
}

LD_EXPORT(void)
LDClientFDv2ModeBuilder_Synchronizer_Streaming(
    LDClientFDv2ModeBuilder b,
    LDClientFDv2StreamingBuilder streaming) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(streaming);

    TO_MODE_BUILDER(b)->Synchronizer(*TO_STREAM_BUILDER(streaming));
    LDClientFDv2StreamingBuilder_Free(streaming);
}

LD_EXPORT(void)
LDClientFDv2ModeBuilder_Synchronizer_Polling(
    LDClientFDv2ModeBuilder b,
    LDClientFDv2PollingBuilder polling) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(polling);

    TO_MODE_BUILDER(b)->Synchronizer(*TO_POLL_BUILDER(polling));
    LDClientFDv2PollingBuilder_Free(polling);
}

LD_EXPORT(void)
LDClientFDv2ModeBuilder_FallbackToFDv1(
    LDClientFDv2ModeBuilder b,
    LDClientFDv2FDv1FallbackBuilder fallback) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(fallback);

    TO_MODE_BUILDER(b)->FallbackToFDv1(*TO_FALLBACK_BUILDER(fallback));
    LDClientFDv2FDv1FallbackBuilder_Free(fallback);
}

LD_EXPORT(void)
LDClientFDv2ModeBuilder_DisableFDv1Fallback(LDClientFDv2ModeBuilder b) {
    LD_ASSERT_NOT_NULL(b);

    TO_MODE_BUILDER(b)->DisableFDv1Fallback();
}

LD_EXPORT(LDClientFDv2StreamingBuilder)
LDClientFDv2StreamingBuilder_New(void) {
    return FROM_STREAM_BUILDER(new FDv2Builder::Streaming());
}

LD_EXPORT(void)
LDClientFDv2StreamingBuilder_InitialReconnectDelayMs(
    LDClientFDv2StreamingBuilder b,
    unsigned int milliseconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_STREAM_BUILDER(b)->InitialReconnectDelay(
        std::chrono::milliseconds{milliseconds});
}

LD_EXPORT(void)
LDClientFDv2StreamingBuilder_BaseURL(LDClientFDv2StreamingBuilder b,
                                     char const* base_url) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(base_url);

    TO_STREAM_BUILDER(b)->BaseUrl(base_url);
}

LD_EXPORT(void)
LDClientFDv2StreamingBuilder_Free(LDClientFDv2StreamingBuilder b) {
    delete TO_STREAM_BUILDER(b);
}

LD_EXPORT(LDClientFDv2PollingBuilder)
LDClientFDv2PollingBuilder_New(void) {
    return FROM_POLL_BUILDER(new FDv2Builder::Polling());
}

LD_EXPORT(void)
LDClientFDv2PollingBuilder_IntervalS(LDClientFDv2PollingBuilder b,
                                     unsigned int seconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_POLL_BUILDER(b)->PollInterval(std::chrono::seconds{seconds});
}

LD_EXPORT(void)
LDClientFDv2PollingBuilder_BaseURL(LDClientFDv2PollingBuilder b,
                                   char const* base_url) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(base_url);

    TO_POLL_BUILDER(b)->BaseUrl(base_url);
}

LD_EXPORT(void)
LDClientFDv2PollingBuilder_Free(LDClientFDv2PollingBuilder b) {
    delete TO_POLL_BUILDER(b);
}

LD_EXPORT(LDClientFDv2FDv1FallbackBuilder)
LDClientFDv2FDv1FallbackBuilder_New(void) {
    return FROM_FALLBACK_BUILDER(new FDv2Builder::FDv1Fallback());
}

LD_EXPORT(void)
LDClientFDv2FDv1FallbackBuilder_IntervalS(LDClientFDv2FDv1FallbackBuilder b,
                                          unsigned int seconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_FALLBACK_BUILDER(b)->PollInterval(std::chrono::seconds{seconds});
}

LD_EXPORT(void)
LDClientFDv2FDv1FallbackBuilder_BaseURL(LDClientFDv2FDv1FallbackBuilder b,
                                        char const* base_url) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(base_url);

    TO_FALLBACK_BUILDER(b)->BaseUrl(base_url);
}

LD_EXPORT(void)
LDClientFDv2FDv1FallbackBuilder_Free(LDClientFDv2FDv1FallbackBuilder b) {
    delete TO_FALLBACK_BUILDER(b);
}

// NOLINTEND OCInconsistentNamingInspection
// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
