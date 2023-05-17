#include <gtest/gtest.h>

#include <launchdarkly/bindings/c/data/evaluation_detail.h>
#include <launchdarkly/data/evaluation_reason.hpp>

TEST(ClientBindings, EvaluationDetailError) {
    using namespace launchdarkly;
    auto reason =
        EvaluationReason(EvaluationReason::ErrorKind::kClientNotReady);
    auto ld_reason = reinterpret_cast<LDEvalReason>(&reason);

    ASSERT_EQ(LDEvalReason_Kind(ld_reason), LD_EVALREASON_ERROR);

    enum LDEvalReason_ErrorKind error_kind;
    ASSERT_TRUE(LDEvalReason_ErrorKind(ld_reason, &error_kind));

    ASSERT_EQ(error_kind, LD_EVALREASON_ERROR_CLIENT_NOT_READY);

    ASSERT_FALSE(LDEvalReason_InExperiment(ld_reason));
}
