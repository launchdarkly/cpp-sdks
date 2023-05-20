#include <gtest/gtest.h>

#include "launchdarkly/bindings/c/data/evaluation_detail.h"
#include "launchdarkly/data/evaluation_detail.hpp"

using namespace launchdarkly;

TEST(EvaluationDetailBindings, EvaluationReasonError) {
    auto reason =
        EvaluationReason(EvaluationReason::ErrorKind::kClientNotReady);
    auto ld_reason = reinterpret_cast<LDEvalReason>(&reason);

    ASSERT_EQ(LDEvalReason_Kind(ld_reason), LD_EVALREASON_ERROR);

    enum LDEvalReason_ErrorKind error_kind;
    ASSERT_TRUE(LDEvalReason_ErrorKind(ld_reason, &error_kind));

    ASSERT_EQ(error_kind, LD_EVALREASON_ERROR_CLIENT_NOT_READY);

    ASSERT_FALSE(LDEvalReason_InExperiment(ld_reason));
}

TEST(EvaluationDetailBindings, EvaluationReasonFallthrough) {
    auto reason = EvaluationReason(EvaluationReason::Kind::kFallthrough,
                                   std::nullopt, std::nullopt, std::nullopt,
                                   std::nullopt, true, std::nullopt);
    auto ld_reason = reinterpret_cast<LDEvalReason>(&reason);

    ASSERT_EQ(LDEvalReason_Kind(ld_reason), LD_EVALREASON_FALLTHROUGH);
    enum LDEvalReason_ErrorKind error_kind;
    ASSERT_FALSE(LDEvalReason_ErrorKind(ld_reason, &error_kind));
    ASSERT_TRUE(LDEvalReason_InExperiment(ld_reason));
}

TEST(EvaluationDetailBindings, EvaluationDetailError) {
    auto detail = CEvaluationDetail(EvaluationDetail<bool>(
        EvaluationReason::ErrorKind::kMalformedFlag, true));

    auto ld_detail = reinterpret_cast<LDEvalDetail>(&detail);

    std::size_t variation_index;
    ASSERT_FALSE(LDEvalDetail_VariationIndex(ld_detail, &variation_index));

    LDEvalReason reason;
    ASSERT_TRUE(LDEvalDetail_Reason(ld_detail, &reason));

    enum LDEvalReason_ErrorKind error_kind;
    ASSERT_TRUE(LDEvalReason_ErrorKind(reason, &error_kind));
    ASSERT_EQ(error_kind, LD_EVALREASON_ERROR_MALFORMED_FLAG);
}

TEST(EvaluationDetailBindings, EvaluationDetailSuccess) {
    auto detail = CEvaluationDetail(EvaluationDetail<bool>(
        true, 42,
        EvaluationReason(EvaluationReason::Kind::kFallthrough, std::nullopt,
                         std::nullopt, std::nullopt, std::nullopt, true,
                         std::nullopt)));

    auto ld_detail = reinterpret_cast<LDEvalDetail>(&detail);

    std::size_t variation_index;
    ASSERT_TRUE(LDEvalDetail_VariationIndex(ld_detail, &variation_index));
    ASSERT_EQ(variation_index, 42);

    LDEvalReason reason;
    ASSERT_TRUE(LDEvalDetail_Reason(ld_detail, &reason));

    enum LDEvalReason_ErrorKind error_kind;
    ASSERT_FALSE(LDEvalReason_ErrorKind(reason, &error_kind));

    ASSERT_TRUE(LDEvalReason_InExperiment(reason));
}
