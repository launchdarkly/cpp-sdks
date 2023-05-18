// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/bindings/c/data/evaluation_detail.h>
#include <launchdarkly/data/evaluation_detail.hpp>
#include <launchdarkly/detail/c_binding_helpers.hpp>

#define TO_DETAIL(ptr) (reinterpret_cast<launchdarkly::CEvaluationDetail*>(ptr))

#define TO_REASON(ptr) (reinterpret_cast<EvaluationReason*>(ptr))
#define FROM_REASON(ptr) (reinterpret_cast<LDEvalReason>(ptr));

using namespace launchdarkly;

LD_EXPORT(void)
LDEvalDetail_Free(LDEvalDetail detail) {
    delete TO_DETAIL(detail);
}

LD_EXPORT(bool)
LDEvalDetail_VariationIndex(LDEvalDetail detail, size_t* out_variation_index) {
    LD_ASSERT_NOT_NULL(detail);
    LD_ASSERT_NOT_NULL(out_variation_index);

    return OptReturn(TO_DETAIL(detail)->variation_index, out_variation_index);
}

LD_EXPORT(bool)
LDEvalDetail_Reason(LDEvalDetail detail, LDEvalReason* out_reason) {
    LD_ASSERT_NOT_NULL(detail);
    LD_ASSERT_NOT_NULL(out_reason);

    return OptReturnReinterpretCast(TO_DETAIL(detail)->reason, out_reason);
}

LD_EXPORT(enum LDEvalReason_Kind)
LDEvalReason_Kind(LDEvalReason reason) {
    LD_ASSERT_NOT_NULL(reason);

    return static_cast<enum LDEvalReason_Kind>(TO_REASON(reason)->kind());
}

LD_EXPORT(bool)
LDEvalReason_ErrorKind(LDEvalReason reason,
                       enum LDEvalReason_ErrorKind* out_error_kind) {
    LD_ASSERT_NOT_NULL(reason);
    LD_ASSERT_NOT_NULL(out_error_kind);

    return OptReturnStaticCast(TO_REASON(reason)->error_kind(), out_error_kind);
}

LD_EXPORT(bool)
LDEvalReason_InExperiment(LDEvalReason reason) {
    LD_ASSERT_NOT_NULL(reason);

    return TO_REASON(reason)->in_experiment();
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
