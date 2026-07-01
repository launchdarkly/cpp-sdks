// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/server_side/bindings/c/config/big_segments_builder/big_segments_builder.h>

#include <launchdarkly/detail/c_binding_helpers.hpp>
#include <launchdarkly/server_side/config/builders/big_segments_builder.hpp>
#include <launchdarkly/server_side/integrations/big_segments/ibig_segment_store.hpp>

#include <chrono>
#include <memory>

using namespace launchdarkly::server_side;
using namespace launchdarkly::server_side::config::builders;

#define TO_BS_BUILDER(ptr) (reinterpret_cast<BigSegmentsBuilder*>(ptr))
#define FROM_BS_BUILDER(ptr) (reinterpret_cast<LDServerBigSegmentsBuilder>(ptr))

LD_EXPORT(LDServerBigSegmentsBuilder)
LDServerBigSegmentsBuilder_New(LDServerBigSegmentStorePtr store) {
    LD_ASSERT_NOT_NULL(store);

    auto* raw_store = reinterpret_cast<integrations::IBigSegmentStore*>(store);
    auto owned = std::shared_ptr<integrations::IBigSegmentStore>(raw_store);
    return FROM_BS_BUILDER(new BigSegmentsBuilder(std::move(owned)));
}

LD_EXPORT(void)
LDServerBigSegmentsBuilder_ContextCacheSize(LDServerBigSegmentsBuilder b,
                                            size_t size) {
    LD_ASSERT_NOT_NULL(b);

    TO_BS_BUILDER(b)->ContextCacheSize(size);
}

LD_EXPORT(void)
LDServerBigSegmentsBuilder_ContextCacheTimeMs(LDServerBigSegmentsBuilder b,
                                              unsigned int milliseconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_BS_BUILDER(b)->ContextCacheTime(std::chrono::milliseconds{milliseconds});
}

LD_EXPORT(void)
LDServerBigSegmentsBuilder_StatusPollIntervalMs(LDServerBigSegmentsBuilder b,
                                                unsigned int milliseconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_BS_BUILDER(b)->StatusPollInterval(
        std::chrono::milliseconds{milliseconds});
}

LD_EXPORT(void)
LDServerBigSegmentsBuilder_StaleAfterMs(LDServerBigSegmentsBuilder b,
                                        unsigned int milliseconds) {
    LD_ASSERT_NOT_NULL(b);

    TO_BS_BUILDER(b)->StaleAfter(std::chrono::milliseconds{milliseconds});
}

LD_EXPORT(void)
LDServerBigSegmentsBuilder_Free(LDServerBigSegmentsBuilder b) {
    delete TO_BS_BUILDER(b);
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
